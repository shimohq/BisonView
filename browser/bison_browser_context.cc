#include "bison/browser/bison_browser_context.h"

#include <utility>

#include "bison/bison_jni_headers/BisonBrowserContext_jni.h"
#include "bison/browser/bison_browser_process.h"
#include "bison/browser/bison_content_browser_client.h"
#include "bison/browser/bison_download_manager_delegate.h"
#include "bison/browser/bison_form_database_service.h"
#include "bison/browser/bison_permission_manager.h"
#include "bison/browser/bison_quota_manager_bridge.h"
#include "bison/browser/bison_resource_context.h"
#include "bison/browser/cookie_manager.h"
#include "bison/browser/network_service/net_helpers.h"
#include "bison/common/bison_features.h"

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/single_thread_task_runner.h"
#include "base/task/post_task.h"
#include "components/autofill/core/browser/autocomplete_history_manager.h"
#include "components/autofill/core/common/autofill_prefs.h"
#include "components/cdm/browser/media_drm_storage_impl.h"
#include "components/keyed_service/core/simple_key_map.h"
#include "components/policy/core/browser/browser_policy_connector_base.h"
#include "components/policy/core/browser/configuration_policy_pref_store.h"
#include "components/policy/core/browser/url_blacklist_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/in_memory_pref_store.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/user_prefs/user_prefs.h"
#include "components/variations/net/variations_http_headers.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cors_exempt_headers.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_switches.h"
#include "media/mojo/buildflags.h"
#include "services/preferences/tracked/segregated_pref_store.h"

using base::FilePath;
using content::BackgroundFetchDelegate;
using content::BackgroundSyncController;
using content::BrowserPluginGuestManager;
using content::BrowserThread;
using content::BrowsingDataRemoverDelegate;
using content::ClientHintsControllerDelegate;
using content::ContentIndexProvider;
using content::DownloadManagerDelegate;
using content::PermissionControllerDelegate;
using content::PushMessagingService;
using content::ResourceContext;
using content::SSLHostStateDelegate;
using content::StorageNotificationService;

namespace bison {

namespace {

const void* const kDownloadManagerDelegateKey = &kDownloadManagerDelegateKey;

BisonBrowserContext* g_browser_context = NULL;
}  // namespace

BisonBrowserContext::BisonBrowserContext()
    : context_storage_path_(GetContextStoragePath()),
      simple_factory_key_(GetPath(), IsOffTheRecord()) {
  DCHECK(!g_browser_context);
  g_browser_context = this;
  SimpleKeyMap::GetInstance()->Associate(this, &simple_factory_key_);

  BrowserContext::Initialize(this, context_storage_path_);

  CreateUserPrefService();

  form_database_service_.reset(
      new BisonFormDatabaseService(context_storage_path_));
}

BisonBrowserContext::~BisonBrowserContext() {
  DCHECK_EQ(this, g_browser_context);
  NotifyWillBeDestroyed(this);
  SimpleKeyMap::GetInstance()->Dissociate(this);
  if (resource_context_) {
    BrowserThread::DeleteSoon(BrowserThread::IO, FROM_HERE,
                              resource_context_.release());
  }
  ShutdownStoragePartitions();

  g_browser_context = NULL;
}
// static
BisonBrowserContext* BisonBrowserContext::GetDefault() {
  // TODO(joth): rather than store in a global here, lookup this instance
  // from the Java-side peer.
  return g_browser_context;
}

// static
BisonBrowserContext* BisonBrowserContext::FromWebContents(
    content::WebContents* web_contents) {
  return static_cast<BisonBrowserContext*>(web_contents->GetBrowserContext());
}

base::FilePath BisonBrowserContext::GetCacheDir() {
  FilePath cache_path;
  if (!base::PathService::Get(base::DIR_CACHE, &cache_path)) {
    NOTREACHED() << "Failed to get app cache directory for Android WebView";
  }
  cache_path = cache_path.Append(FILE_PATH_LITERAL("Default"))
                   .Append(FILE_PATH_LITERAL("HTTP Cache"));
  return cache_path;
}

base::FilePath BisonBrowserContext::GetPrefStorePath() {
  FilePath pref_store_path;
  base::PathService::Get(base::DIR_ANDROID_APP_DATA, &pref_store_path);
  // TODO(amalova): Assign a proper file path for non-default profiles
  // when we support multiple profiles
  pref_store_path =
      pref_store_path.Append(FILE_PATH_LITERAL("Default/Preferences"));

  return pref_store_path;
}

base::FilePath BisonBrowserContext::GetCookieStorePath() {
  return GetCookieManager()->GetCookieStorePath();
}

// static
base::FilePath BisonBrowserContext::GetContextStoragePath() {
  base::FilePath user_data_dir;
  if (!base::PathService::Get(base::DIR_ANDROID_APP_DATA, &user_data_dir)) {
    NOTREACHED() << "Failed to get app data directory for Android WebView";
  }

  user_data_dir = user_data_dir.Append(FILE_PATH_LITERAL("Default"));
  return user_data_dir;
}

// static
void BisonBrowserContext::RegisterPrefs(PrefRegistrySimple* registry) {
  // safe_browsing::RegisterProfilePrefs(registry);

  // Register the Autocomplete Data Retention Policy pref.
  // The default value '0' represents the latest Chrome major version on which
  // the retention policy ran. By setting it to a low default value, we're
  // making sure it runs now (as it only runs once per major version).

  // registry->RegisterIntegerPref(
  //     autofill::prefs::kAutocompleteLastVersionRetentionPolicy, 0);

  // We only use the autocomplete feature of Autofill, which is controlled via
  // the manager_delegate. We don't use the rest of Autofill, which is why it is
  // hardcoded as disabled here.
  // TODO(crbug.com/873740): The following also disables autocomplete.
  // Investigate what the intended behavior is.

  // registry->RegisterBooleanPref(autofill::prefs::kAutofillProfileEnabled,
  //                               false);
  // registry->RegisterBooleanPref(autofill::prefs::kAutofillCreditCardEnabled,
  //                               false);

#if BUILDFLAG(ENABLE_MOJO_CDM)
  cdm::MediaDrmStorageImpl::RegisterProfilePrefs(registry);
#endif
}

void BisonBrowserContext::CreateUserPrefService() {
  auto pref_registry = base::MakeRefCounted<user_prefs::PrefRegistrySyncable>();

  RegisterPrefs(pref_registry.get());

  PrefServiceFactory pref_service_factory;

  std::set<std::string> persistent_prefs;
  // Persisted to avoid having to provision MediaDrm every time the
  // application tries to play protected content after restart.
  persistent_prefs.insert(cdm::prefs::kMediaDrmStorage);

  pref_service_factory.set_user_prefs(base::MakeRefCounted<SegregatedPrefStore>(
      base::MakeRefCounted<InMemoryPrefStore>(),
      base::MakeRefCounted<JsonPrefStore>(GetPrefStorePath()), persistent_prefs,
      mojo::Remote<::prefs::mojom::TrackedPreferenceValidationDelegate>()));

  policy::URLBlacklistManager::RegisterProfilePrefs(pref_registry.get());
  BisonBrowserPolicyConnector* browser_policy_connector =
      BisonBrowserProcess::GetInstance()->browser_policy_connector();
  pref_service_factory.set_managed_prefs(
      base::MakeRefCounted<policy::ConfigurationPolicyPrefStore>(
          browser_policy_connector,
          browser_policy_connector->GetPolicyService(),
          browser_policy_connector->GetHandlerList(),
          policy::POLICY_LEVEL_MANDATORY));

  user_pref_service_ = pref_service_factory.Create(pref_registry);

  // if (IsDefaultBrowserContext()) {
  //   MigrateLocalStatePrefs();
  // }

  user_prefs::UserPrefs::Set(this, user_pref_service_.get());
}

// static
std::vector<std::string> BisonBrowserContext::GetAuthSchemes() {
  std::vector<std::string> supported_schemes = {"basic", "digest", "ntlm",
                                                "negotiate"};
  return supported_schemes;
}

BisonQuotaManagerBridge* BisonBrowserContext::GetQuotaManagerBridge() {
  if (!quota_manager_bridge_.get()) {
    quota_manager_bridge_ = BisonQuotaManagerBridge::Create(this);
  }
  return quota_manager_bridge_.get();
}

autofill::AutocompleteHistoryManager*
BisonBrowserContext::GetAutocompleteHistoryManager() {
  if (!autocomplete_history_manager_) {
    autocomplete_history_manager_ =
        std::make_unique<autofill::AutocompleteHistoryManager>();
    autocomplete_history_manager_->Init(
        form_database_service_->get_autofill_webdata_service(),
        user_pref_service_.get(), IsOffTheRecord());
  }

  return autocomplete_history_manager_.get();
}

CookieManager* BisonBrowserContext::GetCookieManager() {
  // TODO(amalova): create cookie manager for non-default profile
  return CookieManager::GetInstance();
}

base::FilePath BisonBrowserContext::GetPath() {
  return context_storage_path_;
}

bool BisonBrowserContext::IsOffTheRecord() {
  return false;
}

ResourceContext* BisonBrowserContext::GetResourceContext() {
  if (!resource_context_) {
    resource_context_.reset(new BisonResourceContext);
  }
  return resource_context_.get();
}

DownloadManagerDelegate* BisonBrowserContext::GetDownloadManagerDelegate() {
  if (!download_manager_delegate_.get()) {
    download_manager_delegate_.reset(new BisonDownloadManagerDelegate());
  }

  return download_manager_delegate_.get();
}

BrowserPluginGuestManager* BisonBrowserContext::GetGuestManager() {
  return NULL;
}

storage::SpecialStoragePolicy* BisonBrowserContext::GetSpecialStoragePolicy() {
  return NULL;
}

PushMessagingService* BisonBrowserContext::GetPushMessagingService() {
  return NULL;
}

StorageNotificationService*
BisonBrowserContext::GetStorageNotificationService() {
  return nullptr;
}

SSLHostStateDelegate* BisonBrowserContext::GetSSLHostStateDelegate() {
  return nullptr;
}

PermissionControllerDelegate*
BisonBrowserContext::GetPermissionControllerDelegate() {
  if (!permission_manager_.get())
    permission_manager_.reset(new BisonPermissionManager());
  return permission_manager_.get();
}

ClientHintsControllerDelegate*
BisonBrowserContext::GetClientHintsControllerDelegate() {
  return nullptr;
}

BackgroundFetchDelegate* BisonBrowserContext::GetBackgroundFetchDelegate() {
  return nullptr;
}

content::BackgroundSyncController*
BisonBrowserContext::GetBackgroundSyncController() {
  return nullptr;
}

BrowsingDataRemoverDelegate*
BisonBrowserContext::GetBrowsingDataRemoverDelegate() {
  return nullptr;
}

network::mojom::NetworkContextParamsPtr
BisonBrowserContext::GetNetworkContextParams(
    bool in_memory,
    const base::FilePath& relative_partition_path) {
  VLOG(0) << "GetNetworkContextParams";
  network::mojom::NetworkContextParamsPtr context_params =
      network::mojom::NetworkContextParams::New();
  context_params->user_agent = bison::GetUserAgent();

  // TODO(ntfschr): set this value to a proper value based on the user's
  // preferred locales (http://crbug.com/898555). For now, set this to
  // "en-US,en" instead of "en-us,en", since Android guarantees region codes
  // will be uppercase.
  context_params->accept_language =
      net::HttpUtil::GenerateAcceptLanguageHeader("en-US,en");

  // HTTP cache
  context_params->http_cache_enabled = true;
  context_params->http_cache_max_size = GetHttpCacheSize();
  context_params->http_cache_path = GetCacheDir();

  VLOG(0) << "http cacah path:" << GetCacheDir();
  // VLOG(0) << "cookie path:" << BisonBrowserContext::GetCookieStorePath();

  // jiang ?? 这里会...
  // // BisonView should persist and restore cookies between app sessions
  // // (including session cookies).
  // context_params->cookie_path = BisonBrowserContext::GetCookieStorePath();
  // context_params->restore_old_session_cookies = true;
  // context_params->persist_session_cookies = true;
  // context_params->cookie_manager_params =
  //     network::mojom::CookieManagerParams::New();
  // context_params->cookie_manager_params->allow_file_scheme_cookies =
  //     GetCookieManager()->AllowFileSchemeCookies();

  context_params->initial_ssl_config = network::mojom::SSLConfig::New();
  // Allow SHA-1 to be used for locally-installed trust anchors, as WebView
  // should behave like the Android system would.
  context_params->initial_ssl_config->sha1_local_anchors_enabled = true;
  // Do not enforce the Legacy Symantec PKI policies outlined in
  // https://security.googleblog.com/2017/09/chromes-plan-to-distrust-symantec.html,
  // defer to the Android system.
  context_params->initial_ssl_config->symantec_enforcement_disabled = true;

  // WebView does not currently support Certificate Transparency
  // (http://crbug.com/921750).
  context_params->enforce_chrome_ct_policy = false;

  // WebView does not support ftp yet.
  context_params->enable_ftp_url_support = false;

  context_params->enable_brotli =
      base::FeatureList::IsEnabled(bison::features::kWebViewBrotliSupport);

  context_params->check_clear_text_permitted =
      BisonContentBrowserClient::get_check_cleartext_permitted();

  // Update the cors_exempt_header_list to include internally-added headers, to
  // avoid triggering CORS checks.
  content::UpdateCorsExemptHeader(context_params.get());
  // variations::UpdateCorsExemptHeaderForVariations(context_params.get());

  // Add proxy settings
  BisonProxyConfigMonitor::GetInstance()->AddProxyToNetworkContextParams(
      context_params);

  return context_params;
}

base::android::ScopedJavaLocalRef<jobject>
JNI_BisonBrowserContext_GetDefaultJava(JNIEnv* env) {
  return g_browser_context->GetJavaBrowserContext();
}

base::android::ScopedJavaLocalRef<jobject>
BisonBrowserContext::GetJavaBrowserContext() {
  if (!obj_) {
    JNIEnv* env = base::android::AttachCurrentThread();
    obj_ = Java_BisonBrowserContext_create(
        env, reinterpret_cast<intptr_t>(this), IsDefaultBrowserContext());
  }
  return base::android::ScopedJavaLocalRef<jobject>(obj_);
}

jlong BisonBrowserContext::GetQuotaManagerBridge(JNIEnv* env) {
  return reinterpret_cast<intptr_t>(GetQuotaManagerBridge());
}

}  // namespace bison
