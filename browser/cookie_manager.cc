#include "bison/browser/cookie_manager.h"

#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "bison/browser/bv_browser_context.h"
#include "bison/browser/bv_cookie_access_policy.h"
#include "bison/bison_jni_headers/BvCookieManager_jni.h"
#include "base/android/build_info.h"
#include "base/android/callback_android.h"
#include "base/android/jni_string.h"
#include "base/android/path_utils.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/containers/circular_deque.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/no_destructor.h"
#include "base/optional.h"
#include "base/path_service.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cookie_store_factory.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/cookie_inclusion_status.h"
#include "net/cookies/cookie_monster.h"
#include "net/cookies/cookie_options.h"
#include "net/cookies/cookie_store.h"
#include "net/cookies/cookie_util.h"
#include "net/cookies/parsed_cookie.h"
#include "net/url_request/url_request_context.h"
#include "services/network/cookie_access_delegate_impl.h"
#include "services/network/network_service.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"
#include "url/url_constants.h"

using base::WaitableEvent;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertJavaStringToUTF16;
using base::android::JavaParamRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;
using net::CookieList;

// In the future, we may instead want to inject an explicit CookieStore
// dependency into this object during process initialization to avoid
// depending on the URLRequestContext.
// See issue http://crbug.com/157683

// On the CookieManager methods without a callback and methods with a callback
// when that callback is null can be called from any thread, including threads
// without a message loop. Methods with a non-null callback must be called on
// a thread with a running message loop.

namespace bison {

namespace {

void MaybeRunCookieCallback(base::OnceCallback<void(bool)> callback,
                            const bool& result) {
  if (callback)
    std::move(callback).Run(result);
}

const char kSecureCookieHistogramName[] = "BisonView.SecureCookieAction";

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class SecureCookieAction {
  kInvalidUrl = 0,
  kAlreadySecureScheme = 1,
  kInvalidCookie = 2,
  kNotASecureCookie = 3,
  kFixedUp = 4,
  kDisallowedAndroidR = 5,
  kMaxValue = kDisallowedAndroidR,
};

// Since this function parses the set-cookie line into a ParsedCookie, it is
// convenient to hook into here to get the SameSite value from the parsed
// cookie for histogramming.
GURL MaybeFixUpSchemeForSecureCookieAndGetSameSite(
    const GURL& host,
    const std::string& value,
    bool workaround_http_secure_cookies,
    net::CookieSameSiteString* samesite_out,
    bool* should_allow_cookie) {
  net::ParsedCookie parsed_cookie(value);

  // Grab the SameSite value for histogramming.
  DCHECK(samesite_out);
  parsed_cookie.SameSite(samesite_out);

  *should_allow_cookie = true;

  // Log message for catching strict secure cookies related bugs.
  // TODO(ntfschr): try to remove this, based on UMA stats
  // (https://crbug.com/933981)
  if (!host.is_valid()) {
    base::UmaHistogramEnumeration(kSecureCookieHistogramName,
                                  SecureCookieAction::kInvalidUrl);
    return host;
  }
  if (host.has_scheme() && !host.SchemeIs(url::kHttpScheme)) {
    base::UmaHistogramEnumeration(kSecureCookieHistogramName,
                                  SecureCookieAction::kAlreadySecureScheme);
    return host;
  }
  if (!parsed_cookie.IsValid()) {
    base::UmaHistogramEnumeration(kSecureCookieHistogramName,
                                  SecureCookieAction::kInvalidCookie);
    return host;
  }
  if (!parsed_cookie.IsSecure()) {
    base::UmaHistogramEnumeration(kSecureCookieHistogramName,
                                  SecureCookieAction::kNotASecureCookie);
    return host;
  }

  LOG(ERROR) << "Strict Secure Cookie policy does not allow setting a "
                "secure cookie for "
             << host.spec()
             << " for apps targeting >= R. Please either use the 'https:' "
                "scheme for this URL or omit the 'Secure' directive in the "
                "cookie value.";
  if (!workaround_http_secure_cookies) {
    // Don't allow setting this cookie if we target >= R.
    *should_allow_cookie = false;
    base::UmaHistogramEnumeration(kSecureCookieHistogramName,
                                  SecureCookieAction::kDisallowedAndroidR);
    return host;
  }

  base::UmaHistogramEnumeration(kSecureCookieHistogramName,
                                SecureCookieAction::kFixedUp);
  GURL::Replacements replace_host;
  replace_host.SetSchemeStr(url::kHttpsScheme);
  return host.ReplaceComponents(replace_host);
}

// Construct a closure which signals a waitable event if and when the closure
// is called the waitable event must still exist.
static base::OnceClosure SignalEventClosure(WaitableEvent* completion) {
  return base::BindOnce(&WaitableEvent::Signal, base::Unretained(completion));
}

static void DiscardBool(base::OnceClosure f, bool b) {
  std::move(f).Run();
}

static base::OnceCallback<void(bool)> BoolCallbackAdapter(base::OnceClosure f) {
  return base::BindOnce(&DiscardBool, std::move(f));
}

static void DiscardInt(base::OnceClosure f, int i) {
  std::move(f).Run();
}

static base::OnceCallback<void(int)> IntCallbackAdapter(base::OnceClosure f) {
  return base::BindOnce(&DiscardInt, std::move(f));
}

// Are cookies allowed for file:// URLs by default?
const bool kDefaultFileSchemeAllowed = false;

}  // namespace

// static
CookieManager* CookieManager::GetInstance() {
  static base::NoDestructor<CookieManager> instance;
  return instance.get();
}

namespace {
base::FilePath GetPathInAppDirectory(std::string path) {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_ANDROID_APP_DATA, &result)) {
    NOTREACHED() << "Failed to get app data directory for Android WebView";
  }
  result = result.Append(FILE_PATH_LITERAL(path));
  return result;
}
}  // namespace

CookieManager::CookieManager()
    : allow_file_scheme_cookies_(kDefaultFileSchemeAllowed),
      cookie_store_created_(false),
      workaround_http_secure_cookies_(
          !base::android::BuildInfo::GetInstance()->targets_at_least_r()),
      cookie_store_client_thread_("CookieMonsterClient"),
      cookie_store_backend_thread_("CookieMonsterBackend"),
      setting_new_mojo_cookie_manager_(false) {
  cookie_store_client_thread_.Start();
  cookie_store_backend_thread_.Start();
  cookie_store_task_runner_ = cookie_store_client_thread_.task_runner();

  // TODO(amalova): initialize cookie_store_path_ for non-default profile
  // Do not migrate cookies for non-default profile.
  cookie_store_path_ = GetPathInAppDirectory("Default/Cookies");
  MigrateCookieStorePath();
}

CookieManager::~CookieManager() {}

void CookieManager::MigrateCookieStorePath() {
  base::FilePath old_cookie_store_path = GetPathInAppDirectory("Cookies");
  base::FilePath old_cookie_journal_path =
      GetPathInAppDirectory("Cookies-journal");
  base::FilePath new_cookie_journal_path =
      GetPathInAppDirectory("Default/Cookies-journal");

  if (base::PathExists(old_cookie_store_path)) {
    base::Move(old_cookie_store_path, cookie_store_path_);
    base::Move(old_cookie_journal_path, new_cookie_journal_path);
  } else {
    // Some users got an incomplete version of this migration where the journal
    // was not moved. Delete the old journal if it exists, as we can't merge
    // them.
    // TODO(torne): remove this in a future release (M81?)
    base::DeleteFile(old_cookie_journal_path, false);
  }
}

// Executes the |task| on |cookie_store_task_runner_| and waits for it to
// complete before returning.
//
// To execute a CookieTask synchronously you must arrange for Signal to be
// called on the waitable event at some point. You can call the bool or int
// versions of ExecCookieTaskSync, these will supply the caller with a dummy
// callback which takes an int/bool, throws it away and calls Signal.
// Alternatively you can call the version which supplies a Closure in which
// case you must call Run on it when you want the unblock the calling code.
//
// Ignore a bool callback.
void CookieManager::ExecCookieTaskSync(
    base::OnceCallback<void(base::OnceCallback<void(bool)>)> task) {
  WaitableEvent completion(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                           base::WaitableEvent::InitialState::NOT_SIGNALED);
  ExecCookieTask(base::BindOnce(
      std::move(task), BoolCallbackAdapter(SignalEventClosure(&completion))));

  // Waiting is necessary when implementing synchronous APIs for the WebView
  // embedder.
  base::ScopedAllowBaseSyncPrimitivesOutsideBlockingScope wait;
  completion.Wait();
}

// Ignore an int callback.
void CookieManager::ExecCookieTaskSync(
    base::OnceCallback<void(base::OnceCallback<void(int)>)> task) {
  WaitableEvent completion(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                           base::WaitableEvent::InitialState::NOT_SIGNALED);
  ExecCookieTask(base::BindOnce(
      std::move(task), IntCallbackAdapter(SignalEventClosure(&completion))));
  base::ScopedAllowBaseSyncPrimitivesOutsideBlockingScope wait;
  completion.Wait();
}

// Call the supplied closure when you want to signal that the blocked code can
// continue.
void CookieManager::ExecCookieTaskSync(
    base::OnceCallback<void(base::OnceClosure)> task) {
  WaitableEvent completion(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                           base::WaitableEvent::InitialState::NOT_SIGNALED);
  ExecCookieTask(
      base::BindOnce(std::move(task), SignalEventClosure(&completion)));
  base::ScopedAllowBaseSyncPrimitivesOutsideBlockingScope wait;
  completion.Wait();
}

// Executes the |task| using |cookie_store_task_runner_|.
void CookieManager::ExecCookieTask(base::OnceClosure task) {
  base::AutoLock lock(task_queue_lock_);
  tasks_.push_back(std::move(task));
  // Unretained is safe, since bison::CookieManager is a singleton we
  // never destroy (we don't need PostTask to do any memory management).
  cookie_store_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&CookieManager::RunPendingCookieTasks,
                                base::Unretained(this)));
}

void CookieManager::RunPendingCookieTasks() {
  DCHECK(cookie_store_task_runner_->RunsTasksInCurrentSequence());
  // Don't do any cookie tasks if in the middle of setting a mojo CookieManager,
  // we'll call this method when that operation is finished.
  if (setting_new_mojo_cookie_manager_)
    return;

  // Copy tasks into temp_queue to minimize the amount of time in the critical
  // section, and to mitigate live-lock issues if these tasks append to the task
  // queue themselves.
  base::circular_deque<base::OnceClosure> temp_queue;
  {
    base::AutoLock lock(task_queue_lock_);
    temp_queue.swap(tasks_);
  }
  while (!temp_queue.empty()) {
    std::move(temp_queue.front()).Run();
    temp_queue.pop_front();
  }
}

base::FilePath CookieManager::GetCookieStorePath() {
  return cookie_store_path_;
}

net::CookieStore* CookieManager::GetCookieStore() {
  DCHECK(cookie_store_task_runner_->RunsTasksInCurrentSequence());

  if (!cookie_store_) {
    content::CookieStoreConfig cookie_config(
        cookie_store_path_, true /* restore_old_session_cookies */,
        true /* persist_session_cookies */);
    cookie_config.client_task_runner = cookie_store_task_runner_;
    cookie_config.background_task_runner =
        cookie_store_backend_thread_.task_runner();

    {
      base::AutoLock lock(allow_file_scheme_cookies_lock_);

      // There are some unknowns about how to correctly handle file:// cookies,
      // and our implementation for this is not robust.  http://crbug.com/582985
      //
      // TODO(mmenke): This call should be removed once we can deprecate and
      // remove the Android WebView 'CookieManager::setAcceptFileSchemeCookies'
      // method. Until then, note that this is just not a great idea.
      cookie_config.cookieable_schemes.insert(
          cookie_config.cookieable_schemes.begin(),
          net::CookieMonster::kDefaultCookieableSchemes,
          net::CookieMonster::kDefaultCookieableSchemes +
              net::CookieMonster::kDefaultCookieableSchemesCount);
      if (allow_file_scheme_cookies_)
        cookie_config.cookieable_schemes.push_back(url::kFileScheme);
      cookie_store_created_ = true;
    }

    cookie_store_ = content::CreateCookieStore(cookie_config, nullptr);
    // Use a CookieAccessDelegate that always returns Legacy mode, for
    // compatibility reasons.
    cookie_store_->SetCookieAccessDelegate(
        std::make_unique<network::CookieAccessDelegateImpl>(
            network::mojom::CookieAccessDelegateType::ALWAYS_LEGACY));
  }

  return cookie_store_.get();
}

network::mojom::CookieManager* CookieManager::GetMojoCookieManager() {
  DCHECK(cookie_store_task_runner_->RunsTasksInCurrentSequence());
  if (!mojo_cookie_manager_.is_bound())
    return nullptr;
  return mojo_cookie_manager_.get();
}

void CookieManager::SetMojoCookieManager(
    mojo::PendingRemote<network::mojom::CookieManager> cookie_manager_remote) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ExecCookieTaskSync(base::BindOnce(&CookieManager::SetMojoCookieManagerAsync,
                                    base::Unretained(this),
                                    std::move(cookie_manager_remote)));
}

void CookieManager::SetMojoCookieManagerAsync(
    mojo::PendingRemote<network::mojom::CookieManager> cookie_manager_remote,
    base::OnceClosure complete) {
  DCHECK(cookie_store_task_runner_->RunsTasksInCurrentSequence());
  setting_new_mojo_cookie_manager_ = true;
  // For simplicity, only permit this method to be called once (otherwise, we
  // must sometimes flush the mojo_cookie_manager_ instead of cookie_store_).
  DCHECK(!mojo_cookie_manager_.is_bound());
  if (!cookie_store_created_) {
    SwapMojoCookieManagerAsync(std::move(cookie_manager_remote),
                               std::move(complete));
    return;
  }

  GetCookieStore()->FlushStore(base::BindOnce(
      &CookieManager::SwapMojoCookieManagerAsync, base::Unretained(this),
      std::move(cookie_manager_remote), std::move(complete)));
}

void CookieManager::SwapMojoCookieManagerAsync(
    mojo::PendingRemote<network::mojom::CookieManager> cookie_manager_remote,
    base::OnceClosure complete) {
  DCHECK(cookie_store_task_runner_->RunsTasksInCurrentSequence());
  mojo_cookie_manager_.Bind(std::move(cookie_manager_remote));
  setting_new_mojo_cookie_manager_ = false;
  std::move(complete).Run();  // unblock content initialization
  RunPendingCookieTasks();
}

void CookieManager::SetWorkaroundHttpSecureCookiesForTesting(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jboolean allow) {
  ExecCookieTaskSync(
      base::BindOnce(&CookieManager::SetWorkaroundHttpSecureCookiesAsyncHelper,
                     base::Unretained(this), allow));
}

void CookieManager::SetWorkaroundHttpSecureCookiesAsyncHelper(
    bool allow,
    base::OnceClosure complete) {
  DCHECK(cookie_store_task_runner_->RunsTasksInCurrentSequence());
  workaround_http_secure_cookies_ = allow;
  std::move(complete).Run();
}

void CookieManager::SetShouldAcceptCookies(JNIEnv* env,
                                           const JavaParamRef<jobject>& obj,
                                           jboolean accept) {
  BvCookieAccessPolicy::GetInstance()->SetShouldAcceptCookies(accept);
}

jboolean CookieManager::GetShouldAcceptCookies(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return BvCookieAccessPolicy::GetInstance()->GetShouldAcceptCookies();
}

void CookieManager::SetCookie(JNIEnv* env,
                              const JavaParamRef<jobject>& obj,
                              const JavaParamRef<jstring>& url,
                              const JavaParamRef<jstring>& value,
                              const JavaParamRef<jobject>& java_callback) {
  DCHECK(java_callback) << "Unexpected null Java callback";
  GURL host(ConvertJavaStringToUTF16(env, url));
  std::string cookie_value(ConvertJavaStringToUTF8(env, value));
  base::OnceCallback<void(bool)> callback =
      base::BindOnce(&base::android::RunBooleanCallbackAndroid,
                     ScopedJavaGlobalRef<jobject>(java_callback));

  ExecCookieTask(base::BindOnce(&CookieManager::SetCookieHelper,
                                base::Unretained(this), host, cookie_value,
                                std::move(callback)));
}

void CookieManager::SetCookieSync(JNIEnv* env,
                                  const JavaParamRef<jobject>& obj,
                                  const JavaParamRef<jstring>& url,
                                  const JavaParamRef<jstring>& value) {
  GURL host(ConvertJavaStringToUTF16(env, url));
  std::string cookie_value(ConvertJavaStringToUTF8(env, value));

  ExecCookieTaskSync(base::BindOnce(&CookieManager::SetCookieHelper,
                                    base::Unretained(this), host,
                                    cookie_value));
}

void CookieManager::SetCookieHelper(const GURL& host,
                                    const std::string& value,
                                    base::OnceCallback<void(bool)> callback) {
  DCHECK(cookie_store_task_runner_->RunsTasksInCurrentSequence());

  net::CookieSameSiteString samesite = net::CookieSameSiteString::kUnspecified;
  bool should_allow_cookie = true;
  const GURL& new_host = MaybeFixUpSchemeForSecureCookieAndGetSameSite(
      host, value, workaround_http_secure_cookies_, &samesite,
      &should_allow_cookie);

  UMA_HISTOGRAM_ENUMERATION(
      "BisonView.CookieManager.SameSiteAttributeValue", samesite);

  net::CookieInclusionStatus status;
  std::unique_ptr<net::CanonicalCookie> cc(
      net::CanonicalCookie::Create(new_host, value, base::Time::Now(),
                                   base::nullopt /* server_time */, &status));

  if (!cc || !should_allow_cookie) {
    MaybeRunCookieCallback(std::move(callback), false);
    return;
  }

  if (cc->SameSite() == net::CookieSameSite::NO_RESTRICTION) {
    UMA_HISTOGRAM_BOOLEAN("BisonView.CookieManager.SameSiteNoneIsSecure",
                          cc->IsSecure());
  }

  // Note: CookieStore and network::CookieManager have different signatures: one
  // accepts a boolean callback while the other (recently) changed to accept a
  // CookieInclusionStatus callback. WebView only cares about boolean success,
  // which is why we use |AdaptCookieInclusionStatusToBool|. This is temporary
  // technical debt until we fully launch the Network Service code path.
  if (GetMojoCookieManager()) {
    // *cc.get() is safe, because network::CookieManager::SetCanonicalCookie
    // will make a copy before our smart pointer goes out of scope.
    GetMojoCookieManager()->SetCanonicalCookie(
        *cc.get(), new_host, net::CookieOptions::MakeAllInclusive(),
        net::cookie_util::AdaptCookieInclusionStatusToBool(
            std::move(callback)));
  } else {
    GetCookieStore()->SetCanonicalCookieAsync(
        std::move(cc), new_host, net::CookieOptions::MakeAllInclusive(),
        net::cookie_util::AdaptCookieInclusionStatusToBool(
            std::move(callback)));
  }
}

ScopedJavaLocalRef<jstring> CookieManager::GetCookie(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& url) {
  GURL host(ConvertJavaStringToUTF16(env, url));

  net::CookieList cookie_list;
  ExecCookieTaskSync(base::BindOnce(&CookieManager::GetCookieListAsyncHelper,
                                    base::Unretained(this), host,
                                    &cookie_list));

  return base::android::ConvertUTF8ToJavaString(
      env, net::CanonicalCookie::BuildCookieLine(cookie_list));
}

void CookieManager::GetCookieListAsyncHelper(const GURL& host,
                                             net::CookieList* result,
                                             base::OnceClosure complete) {
  net::CookieOptions options = net::CookieOptions::MakeAllInclusive();

  if (GetMojoCookieManager()) {
    GetMojoCookieManager()->GetCookieList(
        host, options,
        base::BindOnce(&CookieManager::GetCookieListCompleted,
                       base::Unretained(this), std::move(complete), result));
  } else {
    GetCookieStore()->GetCookieListWithOptionsAsync(
        host, options,
        base::BindOnce(&CookieManager::GetCookieListCompleted,
                       base::Unretained(this), std::move(complete), result));
  }
}

void CookieManager::GetCookieListCompleted(
    base::OnceClosure complete,
    net::CookieList* result,
    const net::CookieAccessResultList& value,
    const net::CookieAccessResultList& excluded_cookies) {
  *result = net::cookie_util::StripAccessResults(value);
  std::move(complete).Run();
}

void CookieManager::RemoveSessionCookies(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jobject>& java_callback) {
  DCHECK(java_callback) << "Unexpected null Java callback";
  base::OnceCallback<void(bool)> callback =
      base::BindOnce(&base::android::RunBooleanCallbackAndroid,
                     ScopedJavaGlobalRef<jobject>(java_callback));

  ExecCookieTask(base::BindOnce(&CookieManager::RemoveSessionCookiesHelper,
                                base::Unretained(this), std::move(callback)));
}

void CookieManager::RemoveSessionCookiesSync(JNIEnv* env,
                                             const JavaParamRef<jobject>& obj) {
  ExecCookieTaskSync(base::BindOnce(&CookieManager::RemoveSessionCookiesHelper,
                                    base::Unretained(this)));
}

void CookieManager::RemoveSessionCookiesHelper(
    base::OnceCallback<void(bool)> callback) {
  if (GetMojoCookieManager()) {
    auto match_session_cookies = network::mojom::CookieDeletionFilter::New();
    match_session_cookies->session_control =
        network::mojom::CookieDeletionSessionControl::SESSION_COOKIES;
    GetMojoCookieManager()->DeleteCookies(
        std::move(match_session_cookies),
        base::BindOnce(&CookieManager::RemoveCookiesCompleted,
                       base::Unretained(this), std::move(callback)));
  } else {
    GetCookieStore()->DeleteSessionCookiesAsync(
        base::BindOnce(&CookieManager::RemoveCookiesCompleted,
                       base::Unretained(this), std::move(callback)));
  }
}

void CookieManager::RemoveCookiesCompleted(
    base::OnceCallback<void(bool)> callback,
    uint32_t num_deleted) {
  std::move(callback).Run(num_deleted > 0u);
}

void CookieManager::RemoveAllCookies(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jobject>& java_callback) {
  DCHECK(java_callback) << "Unexpected null Java callback";

  base::OnceCallback<void(bool)> callback =
      base::BindOnce(&base::android::RunBooleanCallbackAndroid,
                     ScopedJavaGlobalRef<jobject>(java_callback));

  ExecCookieTask(base::BindOnce(&CookieManager::RemoveAllCookiesHelper,
                                base::Unretained(this), std::move(callback)));
}

void CookieManager::RemoveAllCookiesSync(JNIEnv* env,
                                         const JavaParamRef<jobject>& obj) {
  ExecCookieTaskSync(base::BindOnce(&CookieManager::RemoveAllCookiesHelper,
                                    base::Unretained(this)));
}

void CookieManager::RemoveAllCookiesHelper(
    base::OnceCallback<void(bool)> callback) {
  if (GetMojoCookieManager()) {
    // An empty filter matches all cookies.
    auto match_all_cookies = network::mojom::CookieDeletionFilter::New();
    GetMojoCookieManager()->DeleteCookies(
        std::move(match_all_cookies),
        base::BindOnce(&CookieManager::RemoveCookiesCompleted,
                       base::Unretained(this), std::move(callback)));
  } else {
    GetCookieStore()->DeleteAllAsync(
        base::BindOnce(&CookieManager::RemoveCookiesCompleted,
                       base::Unretained(this), std::move(callback)));
  }
}

void CookieManager::RemoveExpiredCookies(JNIEnv* env,
                                         const JavaParamRef<jobject>& obj) {
  // HasCookies will call GetAllCookiesAsync, which in turn will force a GC.
  HasCookies(env, obj);
}

void CookieManager::FlushCookieStore(JNIEnv* env,
                                     const JavaParamRef<jobject>& obj) {
  ExecCookieTaskSync(base::BindOnce(&CookieManager::FlushCookieStoreAsyncHelper,
                                    base::Unretained(this)));
}

void CookieManager::FlushCookieStoreAsyncHelper(base::OnceClosure complete) {
  if (GetMojoCookieManager()) {
    GetMojoCookieManager()->FlushCookieStore(std::move(complete));
  } else {
    GetCookieStore()->FlushStore(std::move(complete));
  }
}

jboolean CookieManager::HasCookies(JNIEnv* env,
                                   const JavaParamRef<jobject>& obj) {
  bool has_cookies;
  ExecCookieTaskSync(base::BindOnce(&CookieManager::HasCookiesAsyncHelper,
                                    base::Unretained(this), &has_cookies));
  return has_cookies;
}

// TODO(kristianm): Simplify this, copying the entire list around
// should not be needed.
void CookieManager::HasCookiesAsyncHelper(bool* result,
                                          base::OnceClosure complete) {
  if (GetMojoCookieManager()) {
    GetMojoCookieManager()->GetAllCookies(
        base::BindOnce(&CookieManager::HasCookiesCompleted,
                       base::Unretained(this), std::move(complete), result));
  } else {
    GetCookieStore()->GetAllCookiesAsync(
        base::BindOnce(&CookieManager::HasCookiesCompleted,
                       base::Unretained(this), std::move(complete), result));
  }
}

void CookieManager::HasCookiesCompleted(base::OnceClosure complete,
                                        bool* result,
                                        const CookieList& cookies) {
  *result = cookies.size() != 0;
  std::move(complete).Run();
}

bool CookieManager::GetAllowFileSchemeCookies() {
  base::AutoLock lock(allow_file_scheme_cookies_lock_);
  return allow_file_scheme_cookies_;
}

jboolean CookieManager::GetAllowFileSchemeCookies(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return GetAllowFileSchemeCookies();
}

void CookieManager::SetAllowFileSchemeCookies(JNIEnv* env,
                                              const JavaParamRef<jobject>& obj,
                                              jboolean allow) {
  ExecCookieTaskSync(
      base::BindOnce(&CookieManager::SetAllowFileSchemeCookiesAsyncHelper,
                     base::Unretained(this), allow));
}

void CookieManager::SetAllowFileSchemeCookiesAsyncHelper(
    bool allow,
    base::OnceClosure complete) {
  DCHECK(cookie_store_task_runner_->RunsTasksInCurrentSequence());
  if (GetMojoCookieManager()) {
    GetMojoCookieManager()->AllowFileSchemeCookies(
        allow,
        base::BindOnce(&CookieManager::SetAllowFileSchemeCookiesCompleted,
                       base::Unretained(this), std::move(complete), allow));
  } else {
    // If we have neither a Network Service CookieManager nor have created the
    // CookieStore, we may modify |allow_file_scheme_cookies_|.
    bool can_change_schemes = !cookie_store_created_;
    SetAllowFileSchemeCookiesCompleted(std::move(complete), allow,
                                       can_change_schemes);
  }
}

void CookieManager::SetAllowFileSchemeCookiesCompleted(
    base::OnceClosure complete,
    bool allow,
    bool can_change_schemes) {
  // Should only update |allow_file_scheme_cookies_| if
  // SetAllowFileSchemeCookiesAsyncHelper said this is OK.
  if (can_change_schemes) {
    base::AutoLock lock(allow_file_scheme_cookies_lock_);
    allow_file_scheme_cookies_ = allow;
  }
  std::move(complete).Run();
}

static jlong JNI_BvCookieManager_GetDefaultCookieManager(JNIEnv* env) {
  return reinterpret_cast<intptr_t>(CookieManager::GetInstance());
}

}  // namespace bison
