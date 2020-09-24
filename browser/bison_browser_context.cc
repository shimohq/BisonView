#include "bison/browser/bison_browser_context.h"

#include <utility>

#include "bison/bison_jni_headers/BisonBrowserContext_jni.h"
#include "bison/browser/bison_content_browser_client.h"
#include "bison/browser/bison_download_manager_delegate.h"
#include "bison/browser/bison_permission_manager.h"
#include "bison/browser/bison_resource_context.h"
#include "bison/browser/cookie_manager.h"
#include "bison/browser/network_service/net_helpers.h"
#include "bison/common/bison_features.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/threading/thread.h"
#include "build/build_config.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/simple_dependency_manager.h"
#include "components/keyed_service/core/simple_factory_key.h"
#include "components/keyed_service/core/simple_key_map.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/variations/net/variations_http_headers.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cors_exempt_headers.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_switches.h"

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
BisonBrowserContext* g_browser_context = NULL;
}  // namespace

// static
BisonBrowserContext* BisonBrowserContext::GetDefault() {
  // TODO(joth): rather than store in a global here, lookup this instance
  // from the Java-side peer.
  return g_browser_context;
}

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

BisonBrowserContext::BisonBrowserContext() {
  DCHECK(!g_browser_context);
  g_browser_context = this;
  InitWhileIOAllowed();
}

BisonBrowserContext::~BisonBrowserContext() {
  DCHECK_EQ(this, g_browser_context);
  NotifyWillBeDestroyed(this);
  g_browser_context = NULL;
  
  // The SimpleDependencyManager should always be passed after the
  // BrowserContextDependencyManager. This is because the KeyedService instances
  // in the BrowserContextDependencyManager's dependency graph can depend on the
  // ones in the SimpleDependencyManager's graph.
  // deps //components/keyed_service/content
  DependencyManager::PerformInterlockedTwoPhaseShutdown(
      BrowserContextDependencyManager::GetInstance(), this,
      SimpleDependencyManager::GetInstance(), key_.get());

  SimpleKeyMap::GetInstance()->Dissociate(this);

  // Need to destruct the ResourceContext before posting tasks which may delete
  // the URLRequestContext because ResourceContext's destructor will remove any
  // outstanding request while URLRequestContext's destructor ensures that there
  // are no more outstanding requests.
  if (resource_context_) {
    BrowserThread::DeleteSoon(BrowserThread::IO, FROM_HERE,
                              resource_context_.release());
  }
  ShutdownStoragePartitions();
}

void BisonBrowserContext::InitWhileIOAllowed() {
  CHECK(base::PathService::Get(base::DIR_ANDROID_APP_DATA, &path_));
  path_ = path_.Append(FILE_PATH_LITERAL("bison"));

  if (!base::PathExists(path_))
    base::CreateDirectory(path_);
  VLOG(0) << "path_:" << path_.value();
  FinishInitWhileIOAllowed();
}

void BisonBrowserContext::FinishInitWhileIOAllowed() {
  BrowserContext::Initialize(this, path_);
  key_ = std::make_unique<SimpleFactoryKey>(path_, IsOffTheRecord());
  SimpleKeyMap::GetInstance()->Associate(this, key_.get());
}

// #if !defined(OS_ANDROID)
// std::unique_ptr<ZoomLevelDelegate>
// BisonBrowserContext::CreateZoomLevelDelegate(
//     const base::FilePath&) {
//   return std::unique_ptr<ZoomLevelDelegate>();
// }
// #endif  // !defined(OS_ANDROID)
// static
std::vector<std::string> BisonBrowserContext::GetAuthSchemes() {
  std::vector<std::string> supported_schemes = {"basic", "digest", "ntlm",
                                                "negotiate"};
  return supported_schemes;
}

CookieManager* BisonBrowserContext::GetCookieManager() {
  // TODO(amalova): create cookie manager for non-default profile
  return CookieManager::GetInstance();
}

base::FilePath BisonBrowserContext::GetPath() {
  return path_;
}

bool BisonBrowserContext::IsOffTheRecord() {
  return false;
}

DownloadManagerDelegate* BisonBrowserContext::GetDownloadManagerDelegate() {
  if (!download_manager_delegate_.get()) {
    download_manager_delegate_.reset(new BisonDownloadManagerDelegate());
    download_manager_delegate_->SetDownloadManager(
        BrowserContext::GetDownloadManager(this));
  }

  return download_manager_delegate_.get();
}

ResourceContext* BisonBrowserContext::GetResourceContext() {
  if (!resource_context_) {
    resource_context_.reset(new BisonResourceContext);
  }
  return resource_context_.get();
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

ContentIndexProvider* BisonBrowserContext::GetContentIndexProvider() {
  // if (!content_index_provider_)
  //   content_index_provider_ =
  //   std::make_unique<WebTestContentIndexProvider>();
  // return content_index_provider_.get();
  VLOG(0) << "jiang GetContentIndexProvider null";
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

}  // namespace bison
