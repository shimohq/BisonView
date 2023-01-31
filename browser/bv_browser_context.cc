#include "bison/browser/bv_browser_context.h"

#include <utility>

#include "bison/bison_jni_headers/BvBrowserContext_jni.h"
#include "bison/browser/bv_browser_process.h"
#include "bison/browser/bv_client_hints_controller_delegate.h"
#include "bison/browser/bv_content_browser_client.h"
#include "bison/browser/bv_download_manager_delegate.h"
#include "bison/browser/bv_form_database_service.h"
#include "bison/browser/bv_permission_manager.h"
#include "bison/browser/bv_quota_manager_bridge.h"
#include "bison/browser/bv_resource_context.h"
#include "bison/browser/cookie_manager.h"
#include "bison/browser/network_service/net_helpers.h"
#include "bison/common/bv_features.h"
#include "bison/common/bv_switches.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/base_paths_posix.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/task/single_thread_task_runner.h"
#include "components/autofill/core/browser/autocomplete_history_manager.h"
#include "components/autofill/core/common/autofill_prefs.h"
#include "components/cdm/browser/media_drm_storage_impl.h"
#include "components/crash/core/common/crash_key.h"
#include "components/download/public/common/in_progress_download_manager.h"
#include "components/keyed_service/core/simple_key_map.h"
#include "components/policy/core/browser/browser_policy_connector_base.h"
#include "components/policy/core/browser/configuration_policy_pref_store.h"
#include "components/policy/core/browser/url_blocklist_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/in_memory_pref_store.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_name_set.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/prefs/segregated_pref_store.h"
#include "components/profile_metrics/browser_profile_type.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/url_formatter/url_fixer.h"
#include "components/user_prefs/user_prefs.h"
#include "components/visitedlink/browser/visitedlink_writer.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_request_utils.h"
#include "content/public/browser/ssl_host_state_delegate.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/zoom_level_delegate.h"
#include "media/mojo/buildflags.h"
#include "net/http/http_util.h"
#include "net/proxy_resolution/proxy_config_service_android.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "services/cert_verifier/public/mojom/cert_verifier_service_factory.mojom.h"
#include "services/network/public/mojom/network_context.mojom.h"

using base::FilePath;
using content::BrowserThread;

namespace bison {

namespace {

const void* const kDownloadManagerDelegateKey = &kDownloadManagerDelegateKey;

BvBrowserContext* g_browser_context = NULL;

bool IgnoreOriginSecurityCheck(const GURL& url) {
  return true;
}

void MigrateProfileData(base::FilePath cache_path,
                        base::FilePath context_storage_path) {
  TRACE_EVENT0("startup", "MigrateProfileData");
  FilePath old_cache_path;
  base::PathService::Get(base::DIR_CACHE, &old_cache_path);
  old_cache_path = old_cache_path.DirName().Append(
      FILE_PATH_LITERAL("org.chromium.android_webview"));

  if (base::PathExists(old_cache_path)) {
    bool success = base::CreateDirectory(cache_path);
    if (success)
      success &= base::Move(old_cache_path, cache_path);
    DCHECK(success);
  }

  base::FilePath old_context_storage_path;
  base::PathService::Get(base::DIR_ANDROID_APP_DATA, &old_context_storage_path);

  if (!base::PathExists(context_storage_path)) {
    base::CreateDirectory(context_storage_path);
  }

  auto migrate_context_storage_data = [&old_context_storage_path,
                                       &context_storage_path](auto& suffix) {
    FilePath old_file = old_context_storage_path.Append(suffix);
    if (base::PathExists(old_file)) {
      FilePath new_file = context_storage_path.Append(suffix);

      if (base::PathExists(new_file)) {
        bool success =
            base::Move(new_file, new_file.AddExtension(".partial-migration"));
        DCHECK(success);
      }
      bool success = base::Move(old_file, new_file);
      DCHECK(success);
    }
  };

  // These were handled in the initial migration
  migrate_context_storage_data("Web Data");
  migrate_context_storage_data("Web Data-journal");
  migrate_context_storage_data("GPUCache");
  migrate_context_storage_data("blob_storage");
  migrate_context_storage_data("Session Storage");

  // These were missed in the initial migration
  migrate_context_storage_data("File System");
  migrate_context_storage_data("IndexedDB");
  migrate_context_storage_data("Local Storage");
  migrate_context_storage_data("QuotaManager");
  migrate_context_storage_data("QuotaManager-journal");
  migrate_context_storage_data("Service Worker");
  migrate_context_storage_data("VideoDecodeStats");
  migrate_context_storage_data("databases");
  migrate_context_storage_data("shared_proto_db");
  migrate_context_storage_data("webrtc_event_logs");
}

}  // namespace

BvBrowserContext::BvBrowserContext()
    : context_storage_path_(GetContextStoragePath()),
      simple_factory_key_(GetPath(), IsOffTheRecord()),
      service_worker_xrw_allowlist_matcher_(
          base::MakeRefCounted<BvContentsOriginMatcher>()) {
  DCHECK(!g_browser_context);

  profile_metrics::SetBrowserProfileType(
      this, profile_metrics::BrowserProfileType::kRegular);
  if (IsDefaultBrowserContext()) {
    MigrateProfileData(GetCacheDir(), GetContextStoragePath());
  }

  g_browser_context = this;
  SimpleKeyMap::GetInstance()->Associate(this, &simple_factory_key_);

  CreateUserPrefService();

  visitedlink_writer_ =
      std::make_unique<visitedlink::VisitedLinkWriter>(this, this, false);
  visitedlink_writer_->Init();

  form_database_service_ =
      std::make_unique<BvFormDatabaseService>(context_storage_path_);

  EnsureResourceContextInitialized();
}

BvBrowserContext::~BvBrowserContext() {
  DCHECK_EQ(this, g_browser_context);
  NotifyWillBeDestroyed();
  SimpleKeyMap::GetInstance()->Dissociate(this);
  ShutdownStoragePartitions();

  g_browser_context = NULL;
}

// static
BvBrowserContext* BvBrowserContext::GetDefault() {
  // TODO(joth): rather than store in a global here, lookup this instance
  // from the Java-side peer.
  return g_browser_context;
}

// static
BvBrowserContext* BvBrowserContext::FromWebContents(
    content::WebContents* web_contents) {
  return static_cast<BvBrowserContext*>(web_contents->GetBrowserContext());
}

base::FilePath BvBrowserContext::GetCacheDir() {
  FilePath cache_path;
  if (!base::PathService::Get(base::DIR_CACHE, &cache_path)) {
    NOTREACHED() << "Failed to get app cache directory for BisonView";
  }
  cache_path = cache_path.Append(FILE_PATH_LITERAL("Default"))
                   .Append(FILE_PATH_LITERAL("HTTP Cache"));
  return cache_path;
}

base::FilePath BvBrowserContext::GetPrefStorePath() {
  FilePath pref_store_path;
  base::PathService::Get(base::DIR_ANDROID_APP_DATA, &pref_store_path);
  // TODO(amalova): Assign a proper file path for non-default profiles
  // when we support multiple profiles
  pref_store_path =
      pref_store_path.Append(FILE_PATH_LITERAL("Default/Preferences"));

  return pref_store_path;
}

base::FilePath BvBrowserContext::GetCookieStorePath() {
  return GetCookieManager()->GetCookieStorePath();
}

// static
base::FilePath BvBrowserContext::GetContextStoragePath() {
  base::FilePath user_data_dir;
  if (!base::PathService::Get(base::DIR_ANDROID_APP_DATA, &user_data_dir)) {
    NOTREACHED() << "Failed to get app data directory for Android WebView";
  }

  user_data_dir = user_data_dir.Append(FILE_PATH_LITERAL("Default"));
  return user_data_dir;
}

// static
void BvBrowserContext::RegisterPrefs(PrefRegistrySimple* registry) {
  // safe_browsing::RegisterProfilePrefs(registry);

  // Register the Autocomplete Data Retention Policy pref.
  // The default value '0' represents the latest Chrome major version on which
  // the retention policy ran. By setting it to a low default value, we're
  // making sure it runs now (as it only runs once per major version).
  registry->RegisterIntegerPref(
      autofill::prefs::kAutocompleteLastVersionRetentionPolicy, 0);

  // We only use the autocomplete feature of Autofill, which is controlled via
  // the manager_delegate. We don't use the rest of Autofill, which is why it is
  // hardcoded as disabled here.
  // TODO(crbug.com/873740): The following also disables autocomplete.
  // Investigate what the intended behavior is.
  registry->RegisterBooleanPref(autofill::prefs::kAutofillProfileEnabled,
                                false);
  registry->RegisterBooleanPref(autofill::prefs::kAutofillCreditCardEnabled,
                                false);

#if BUILDFLAG(ENABLE_MOJO_CDM)
  cdm::MediaDrmStorageImpl::RegisterProfilePrefs(registry);
#endif
}

void BvBrowserContext::CreateUserPrefService() {
  auto pref_registry = base::MakeRefCounted<user_prefs::PrefRegistrySyncable>();

  RegisterPrefs(pref_registry.get());

  PrefServiceFactory pref_service_factory;

  PrefNameSet persistent_prefs;
  // Persisted to avoid having to provision MediaDrm every time the
  // application tries to play protected content after restart.
  persistent_prefs.insert(cdm::prefs::kMediaDrmStorage);

  pref_service_factory.set_user_prefs(base::MakeRefCounted<SegregatedPrefStore>(
      base::MakeRefCounted<InMemoryPrefStore>(),
      base::MakeRefCounted<JsonPrefStore>(GetPrefStorePath()),
      std::move(persistent_prefs)));

  policy::URLBlocklistManager::RegisterProfilePrefs(pref_registry.get());
  BvBrowserPolicyConnector* browser_policy_connector =
      BvBrowserProcess::GetInstance()->browser_policy_connector();
  pref_service_factory.set_managed_prefs(
      base::MakeRefCounted<policy::ConfigurationPolicyPrefStore>(
          browser_policy_connector,
          browser_policy_connector->GetPolicyService(),
          browser_policy_connector->GetHandlerList(),
          policy::POLICY_LEVEL_MANDATORY));

  user_pref_service_ = pref_service_factory.Create(pref_registry);

  if (IsDefaultBrowserContext()) {
    MigrateLocalStatePrefs();
  }

  user_prefs::UserPrefs::Set(this, user_pref_service_.get());
}

void BvBrowserContext::MigrateLocalStatePrefs() {
  PrefService* local_state = BvBrowserProcess::GetInstance()->local_state();
  if (!local_state->HasPrefPath(cdm::prefs::kMediaDrmStorage)) {
    return;
  }

  user_pref_service_->Set(cdm::prefs::kMediaDrmStorage,
                          local_state->GetValue(cdm::prefs::kMediaDrmStorage));
  local_state->ClearPref(cdm::prefs::kMediaDrmStorage);
}

// static
std::vector<std::string> BvBrowserContext::GetAuthSchemes() {
  std::vector<std::string> supported_schemes = {"basic", "digest", "ntlm",
                                                "negotiate"};
  return supported_schemes;
}

void BvBrowserContext::AddVisitedURLs(const std::vector<GURL>& urls) {
  DCHECK(visitedlink_writer_);
  visitedlink_writer_->AddURLs(urls);
}

BvQuotaManagerBridge* BvBrowserContext::GetQuotaManagerBridge() {
  if (!quota_manager_bridge_.get()) {
    quota_manager_bridge_ = BvQuotaManagerBridge::Create(this);
  }
  return quota_manager_bridge_.get();
}

BvFormDatabaseService* BvBrowserContext::GetFormDatabaseService() {
  return form_database_service_.get();
}

autofill::AutocompleteHistoryManager*
BvBrowserContext::GetAutocompleteHistoryManager() {
  if (!autocomplete_history_manager_) {
    autocomplete_history_manager_ =
        std::make_unique<autofill::AutocompleteHistoryManager>();
    autocomplete_history_manager_->Init(
        form_database_service_->get_autofill_webdata_service(),
        user_pref_service_.get(), IsOffTheRecord());
  }

  return autocomplete_history_manager_.get();
}

CookieManager* BvBrowserContext::GetCookieManager() {
  // TODO(amalova): create cookie manager for non-default profile
  return CookieManager::GetInstance();
}

base::FilePath BvBrowserContext::GetPath() {
  return context_storage_path_;
}

bool BvBrowserContext::IsOffTheRecord() {
  return false;
}

content::ResourceContext* BvBrowserContext::GetResourceContext() {
  if (!resource_context_) {
    resource_context_ = std::make_unique<BvResourceContext>();
  }
  return resource_context_.get();
}

content::DownloadManagerDelegate*
BvBrowserContext::GetDownloadManagerDelegate() {
  if (!GetUserData(kDownloadManagerDelegateKey)) {
    SetUserData(kDownloadManagerDelegateKey,
                std::make_unique<BvDownloadManagerDelegate>());
  }
  return static_cast<BvDownloadManagerDelegate*>(
      GetUserData(kDownloadManagerDelegateKey));
}

content::BrowserPluginGuestManager* BvBrowserContext::GetGuestManager() {
  return NULL;
}

storage::SpecialStoragePolicy* BvBrowserContext::GetSpecialStoragePolicy() {
  return NULL;
}

content::PlatformNotificationService*
BvBrowserContext::GetPlatformNotificationService() {
  return nullptr;
}

content::PushMessagingService* BvBrowserContext::GetPushMessagingService() {
  return NULL;
}

content::StorageNotificationService* BvBrowserContext::GetStorageNotificationService() {
  return nullptr;
}

content::SSLHostStateDelegate* BvBrowserContext::GetSSLHostStateDelegate() {
  if (!ssl_host_state_delegate_.get()) {
    ssl_host_state_delegate_ = std::make_unique<BvSSLHostStateDelegate>();
  }
  return ssl_host_state_delegate_.get();
}

content::PermissionControllerDelegate*
BvBrowserContext::GetPermissionControllerDelegate() {
  if (!permission_manager_.get())
    permission_manager_ = std::make_unique<BvPermissionManager>();
  return permission_manager_.get();
}

content::ClientHintsControllerDelegate*
BvBrowserContext::GetClientHintsControllerDelegate() {
  if (!client_hints_controller_delegate_.get()) {
    client_hints_controller_delegate_ =
        std::make_unique<BvClientHintsControllerDelegate>();
  }
  return client_hints_controller_delegate_.get();
}

content::BackgroundFetchDelegate*
BvBrowserContext::GetBackgroundFetchDelegate() {
  return nullptr;
}

content::BackgroundSyncController*
BvBrowserContext::GetBackgroundSyncController() {
  return nullptr;
}

content::BrowsingDataRemoverDelegate*
BvBrowserContext::GetBrowsingDataRemoverDelegate() {
  return nullptr;
}

content::ReduceAcceptLanguageControllerDelegate*
BvBrowserContext::GetReduceAcceptLanguageControllerDelegate() {
  return nullptr;
}

download::InProgressDownloadManager*
BvBrowserContext::RetriveInProgressDownloadManager() {
  return new download::InProgressDownloadManager(
      nullptr, base::FilePath(), nullptr,
      base::BindRepeating(&IgnoreOriginSecurityCheck),
      base::BindRepeating(&content::DownloadRequestUtils::IsURLSafe),
      /*wake_lock_provider_binder*/ base::NullCallback());
}

std::unique_ptr<content::ZoomLevelDelegate>
BvBrowserContext::CreateZoomLevelDelegate(
    const base::FilePath& partition_path) {
  return nullptr;
}

void BvBrowserContext::RebuildTable(
    const scoped_refptr<URLEnumerator>& enumerator) {
  // Android WebView rebuilds from WebChromeClient.getVisitedHistory. The client
  // can change in the lifetime of this WebView and may not yet be set here.
  // Therefore this initialization path is not used.
  enumerator->OnComplete(true);
}

void BvBrowserContext::SetExtendedReportingAllowed(bool allowed) {
  user_pref_service_->SetBoolean(
      ::prefs::kSafeBrowsingExtendedReportingOptInAllowed, allowed);
}

void BvBrowserContext::ConfigureNetworkContextParams(
    bool in_memory,
    const base::FilePath& relative_partition_path,
    network::mojom::NetworkContextParams* context_params,
    cert_verifier::mojom::CertVerifierCreationParams*
        cert_verifier_creation_params) {
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
  context_params->http_cache_directory = GetCacheDir();

  // BisonView should persist and restore cookies between app sessions
  // (including session cookies).
  context_params->file_paths = network::mojom::NetworkContextFilePaths::New();
  base::FilePath cookie_path = BvBrowserContext::GetCookieStorePath();
  context_params->file_paths->data_directory = cookie_path.DirName();
  context_params->file_paths->cookie_database_name = cookie_path.BaseName();
  context_params->restore_old_session_cookies = true;
  context_params->persist_session_cookies = true;
  context_params->cookie_manager_params =
      network::mojom::CookieManagerParams::New();
  context_params->cookie_manager_params->allow_file_scheme_cookies =
      GetCookieManager()->GetAllowFileSchemeCookies();
  context_params->cookie_manager_params->cookie_access_delegate_type =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kWebViewEnableModernCookieSameSite)
          ? network::mojom::CookieAccessDelegateType::ALWAYS_NONLEGACY
          : network::mojom::CookieAccessDelegateType::ALWAYS_LEGACY;

  context_params->initial_ssl_config = network::mojom::SSLConfig::New();
  // Allow SHA-1 to be used for locally-installed trust anchors, as WebView
  // should behave like the Android system would.
  context_params->initial_ssl_config->sha1_local_anchors_enabled = true;
  // Do not enforce the Legacy Symantec PKI policies outlined in
  // https://security.googleblog.com/2017/09/chromes-plan-to-distrust-symantec.html,
  // defer to the Android system.
  context_params->initial_ssl_config->symantec_enforcement_disabled = true;
  // Do not enforce Legacy TLS removal if support is still enabled.
  if (base::FeatureList::IsEnabled(
          bison::features::kWebViewLegacyTlsSupport)) {
    context_params->initial_ssl_config->version_min =
        network::mojom::SSLVersion::kTLS1;
  }

  //  not currently support Certificate Transparency
  // (http://crbug.com/921750).
  context_params->enforce_chrome_ct_policy = false;

  context_params->enable_brotli = base::FeatureList::IsEnabled(
      bison::features::kWebViewBrotliSupport);

  context_params->check_clear_text_permitted =
      BvContentBrowserClient::get_check_cleartext_permitted();

  // Add proxy settings
  BvProxyConfigMonitor::GetInstance()->AddProxyToNetworkContextParams(
      context_params);
}

base::android::ScopedJavaLocalRef<jobject> JNI_BvBrowserContext_GetDefaultJava(
    JNIEnv* env) {
  return g_browser_context->GetJavaBrowserContext();
}

base::android::ScopedJavaLocalRef<jobject>
BvBrowserContext::GetJavaBrowserContext() {
  if (!obj_) {
    JNIEnv* env = base::android::AttachCurrentThread();
    obj_ = Java_BvBrowserContext_create(env, reinterpret_cast<intptr_t>(this),
                                        IsDefaultBrowserContext());
  }
  return base::android::ScopedJavaLocalRef<jobject>(obj_);
}

jlong BvBrowserContext::GetQuotaManagerBridge(JNIEnv* env) {
  return reinterpret_cast<intptr_t>(GetQuotaManagerBridge());
}

scoped_refptr<BvContentsOriginMatcher>
BvBrowserContext::service_worker_xrw_allowlist_matcher() {
  return service_worker_xrw_allowlist_matcher_;
}

}  // namespace bison
