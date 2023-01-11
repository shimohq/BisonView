#include "bison/browser/bv_content_browser_client.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "bison/browser/bv_browser_context.h"
#include "bison/browser/bv_browser_main_parts.h"
#include "bison/browser/bv_content_renderer_overlay_manifest.h"
#include "bison/browser/bv_contents.h"
#include "bison/browser/bv_contents_client_bridge.h"
#include "bison/browser/bv_contents_io_thread_client.h"
#include "bison/browser/bv_cookie_access_policy.h"
#include "bison/browser/bv_devtools_manager_delegate.h"
#include "bison/browser/bv_feature_list_creator.h"
#include "bison/browser/bv_http_auth_handler.h"
#include "bison/browser/bv_quota_permission_context.h"
#include "bison/browser/bv_resource_context.h"
#include "bison/browser/bv_settings.h"
#include "bison/browser/cookie_manager.h"
#include "bison/browser/network_service/bv_proxying_restricted_cookie_manager.h"
#include "bison/browser/network_service/bv_proxying_url_loader_factory.h"
#include "bison/browser/network_service/bv_url_loader_throttle.h"
#include "bison/browser/tracing/bv_tracing_delegate.h"
#include "bison/common/bv_descriptors.h"
#include "bison/common/bv_features.h"
#include "bison/common/mojom/render_message_filter.mojom.h"
#include "bison/common/url_constants.h"

#include "base/android/build_info.h"
#include "base/android/locale_utils.h"
#include "base/base_paths_android.h"
#include "base/base_switches.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/files/scoped_file.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "build/build_config.h"
#include "components/cdm/browser/cdm_message_filter_android.h"
#include "components/crash/content/browser/crash_handler_host_linux.h"
#include "components/embedder_support/user_agent_utils.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#include "components/page_load_metrics/browser/metrics_navigation_throttle.h"
#include "components/page_load_metrics/browser/metrics_web_contents_observer.h"
#include "components/policy/content/policy_blocklist_navigation_throttle.h"
#include "components/policy/core/browser/browser_policy_connector_base.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_associated_interface.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "content/public/browser/file_url_loader.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/shared_cors_origin_access_list.h"
#include "content/public/browser/site_isolation_policy.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_descriptors.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/user_agent.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "net/android/network_library.h"
#include "net/cookies/site_for_cookies.h"
#include "net/http/http_util.h"
#include "net/net_buildflags.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "net/ssl/ssl_info.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "services/network/network_service.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/cookie_manager.mojom-forward.h"
#include "services/network/public/mojom/fetch_api.mojom.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/resource/resource_bundle_android.h"
#include "ui/display/util/display_util.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/resources/grit/ui_resources.h"

using content::BrowserThread;
using content::WebContents;

namespace bison {

namespace {
static bool g_should_create_thread_pool = true;
#if DCHECK_IS_ON()
// A boolean value to determine if the NetworkContext has been created yet. This
// exists only to check correctness: g_check_cleartext_permitted may only be set
// before the NetworkContext has been created (otherwise,
// g_check_cleartext_permitted won't have any effect).
bool g_created_network_context_params = false;
#endif

// On apps targeting API level O or later, check cleartext is enforced.
bool g_check_cleartext_permitted = false;

class BvContentsMessageFilter
    : public content::BrowserMessageFilter,
      public content::BrowserAssociatedInterface<mojom::RenderMessageFilter> {
 public:
  explicit BvContentsMessageFilter(int process_id);

  BvContentsMessageFilter(const BvContentsMessageFilter&) = delete;
  BvContentsMessageFilter& operator=(const BvContentsMessageFilter&) = delete;

  // BrowserMessageFilter methods.
  bool OnMessageReceived(const IPC::Message& message) override;

  // mojom::RenderMessageFilter overrides:
  void SubFrameCreated(int parent_render_frame_id,
                       int child_render_frame_id) override;

 private:
  ~BvContentsMessageFilter() override;

  int process_id_;
};

BvContentsMessageFilter::BvContentsMessageFilter(int process_id)
    : content::BrowserAssociatedInterface<mojom::RenderMessageFilter>(this),
      process_id_(process_id) {}

BvContentsMessageFilter::~BvContentsMessageFilter() = default;

bool BvContentsMessageFilter::OnMessageReceived(const IPC::Message& message) {
  return false;
}

void BvContentsMessageFilter::SubFrameCreated(int parent_render_frame_id,
                                              int child_render_frame_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  BvContentsIoThreadClient::SubFrameCreated(process_id_, parent_render_frame_id,
                                            child_render_frame_id);
}

}  // namespace

std::string GetProduct() {
  return embedder_support::GetProductAndVersion();
}

std::string GetUserAgent() {
  std::string product = "Version/4.0 Bison/1.2.1 " + GetProduct();
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kUseMobileUserAgent)) {
    product += " Mobile";
  }
  return content::BuildUserAgentFromProductAndExtraOSInfo(
      product, "; wv", content::IncludeAndroidBuildNumber::Include);
}

// TODO(yirui): can use similar logic as in PrependToAcceptLanguagesIfNecessary
// in chrome/browser/android/preferences/pref_service_bridge.cc
// static
std::string BvContentBrowserClient::GetAcceptLangsImpl() {
  // Start with the current locale(s) in BCP47 format.
  std::string locales_string = BvContents::GetLocaleList();

  // If accept languages do not contain en-US, add in en-US which will be
  // used with a lower q-value.
  if (locales_string.find("en-US") == std::string::npos)
    locales_string += ",en-US";
  return locales_string;
}

// static
void BvContentBrowserClient::set_check_cleartext_permitted(bool permitted) {
#if DCHECK_IS_ON()
  DCHECK(!g_created_network_context_params);
#endif
  g_check_cleartext_permitted = permitted;
}

bool BvContentBrowserClient::get_check_cleartext_permitted() {
  return g_check_cleartext_permitted;
}

BvContentBrowserClient::BvContentBrowserClient(
    BvFeatureListCreator* feature_list_creator)
    : sniff_file_urls_(BvSettings::GetAllowSniffingFileUrls()),
      bison_feature_list_creator_(feature_list_creator) {
  DCHECK(bison_feature_list_creator_);
}

BvContentBrowserClient::~BvContentBrowserClient() {}

void BvContentBrowserClient::OnNetworkServiceCreated(
    network::mojom::NetworkService* network_service) {
  content::GetNetworkService()->SetUpHttpAuth(
      network::mojom::HttpAuthStaticParams::New());
  content::GetNetworkService()->ConfigureHttpAuthPrefs(
      BvBrowserProcess::GetInstance()->CreateHttpAuthDynamicParams());
}

void BvContentBrowserClient::ConfigureNetworkContextParams(
    content::BrowserContext* context,
    bool in_memory,
    const base::FilePath& relative_partition_path,
    network::mojom::NetworkContextParams* network_context_params,
    cert_verifier::mojom::CertVerifierCreationParams*
        cert_verifier_creation_params) {
  DCHECK(context);

  content::GetNetworkService()->ConfigureHttpAuthPrefs(
      BvBrowserProcess::GetInstance()->CreateHttpAuthDynamicParams());

  BvBrowserContext* bv_context = static_cast<BvBrowserContext*>(context);
  bv_context->ConfigureNetworkContextParams(in_memory, relative_partition_path,
                                            network_context_params,
                                            cert_verifier_creation_params);

  mojo::PendingRemote<network::mojom::CookieManager> cookie_manager_remote;
  network_context_params->cookie_manager =
      cookie_manager_remote.InitWithNewPipeAndPassReceiver();

#if DCHECK_IS_ON()
  g_created_network_context_params = true;
#endif

  // Pass the mojo::PendingRemote<network::mojom::CookieManager> to
  // android_webview::CookieManager, so it can implement its APIs with this mojo
  // CookieManager.
  bv_context->GetCookieManager()->SetMojoCookieManager(
      std::move(cookie_manager_remote));
}

BvBrowserContext* BvContentBrowserClient::InitBrowserContext() {
  browser_context_ = std::make_unique<BvBrowserContext>();
  return browser_context_.get();
}

std::unique_ptr<content::BrowserMainParts>
BvContentBrowserClient::CreateBrowserMainParts(bool is_integration_test) {
  return std::make_unique<BvBrowserMainParts>(this);
}

std::unique_ptr<content::WebContentsViewDelegate>
BvContentBrowserClient::GetWebContentsViewDelegate(WebContents* web_contents) {
  return nullptr;
}

void BvContentBrowserClient::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  // Grant content: scheme access to the whole renderer process, since weimpose
  // per-view access checks, and access is granted by default (see
  // BvSettings.mAllowContentUrlAccess).
  content::ChildProcessSecurityPolicy::GetInstance()->GrantRequestScheme(
      host->GetID(), url::kContentScheme);

  host->AddFilter(new BvContentsMessageFilter(host->GetID()));
  // WebView always allows persisting data.
  host->AddFilter(new cdm::CdmMessageFilterAndroid(true, false));
}

bool BvContentBrowserClient::IsExplicitNavigation(
    ui::PageTransition transition) {
  return ui::PageTransitionCoreTypeIs(transition, ui::PAGE_TRANSITION_TYPED);
}

bool BvContentBrowserClient::IsHandledURL(const GURL& url) {
  if (!url.is_valid()) {
    // We handle error cases.
    return true;
  }

  const std::string scheme = url.scheme();
  DCHECK_EQ(scheme, base::ToLowerASCII(scheme));
  static const char* const kProtocolList[] = {
    url::kHttpScheme,
    url::kHttpsScheme,
#if BUILDFLAG(ENABLE_WEBSOCKETS)
    url::kWsScheme,
    url::kWssScheme,
#endif  // BUILDFLAG(ENABLE_WEBSOCKETS)
    url::kDataScheme,
    url::kBlobScheme,
    url::kFileSystemScheme,
    content::kChromeUIScheme,
    url::kContentScheme,
  };
  if (scheme == url::kFileScheme) {
    // Return false for the "special" file URLs, so they can be loaded
    // even if access to file: scheme is not granted to the child process.
    return !IsAndroidSpecialFileUrl(url);
  }
  for (const char* supported_protocol : kProtocolList) {
    if (scheme == supported_protocol)
      return true;
  }
  return false;
}

bool BvContentBrowserClient::ForceSniffingFileUrlsForHtml() {
  return sniff_file_urls_;
}

void BvContentBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int child_process_id) {
  if (!command_line->HasSwitch(switches::kSingleProcess)) {
    // The only kind of a child process WebView can have is renderer or utility.
    std::string process_type =
        command_line->GetSwitchValueASCII(switches::kProcessType);
    VLOG(0) << "process_type :" << process_type;
    // DCHECK(process_type == switches::kRendererProcess ||
    //        process_type == switches::kUtilityProcess)
    //     << process_type;
    // Pass crash reporter enabled state to renderer processes.
    if (base::CommandLine::ForCurrentProcess()->HasSwitch(
            ::switches::kEnableCrashReporter)) {
      //command_line->AppendSwitch(::switches::kEnableCrashReporter);
      VLOG(0) << "HasSwitch" << ::switches::kEnableCrashReporter;
    }
    command_line->AppendSwitch(::switches::kEnableCrashReporter);
    if (base::CommandLine::ForCurrentProcess()->HasSwitch(
            ::switches::kEnableCrashReporterForTesting)) {
      //command_line->AppendSwitch(::switches::kEnableCrashReporterForTesting);
      VLOG(0) << "HasSwitch" << ::switches::kEnableCrashReporterForTesting;
    }
    command_line->AppendSwitch(::switches::kEnableCrashReporterForTesting);
  }
}

std::string BvContentBrowserClient::GetApplicationLocale() {
  return base::android::GetDefaultLocaleString();
}

std::string BvContentBrowserClient::GetAcceptLangs(
    content::BrowserContext* context) {
  return GetAcceptLangsImpl();
}

gfx::ImageSkia BvContentBrowserClient::GetDefaultFavicon() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  // TODO(boliu): Bundle our own default favicon?
  return rb.GetImageNamed(IDR_DEFAULT_FAVICON).AsImageSkia();
}

 scoped_refptr<content::QuotaPermissionContext>
 BvContentBrowserClient::CreateQuotaPermissionContext() {
   return new BvQuotaPermissionContext();
 }

content::GeneratedCodeCacheSettings
BvContentBrowserClient::GetGeneratedCodeCacheSettings(
    content::BrowserContext* context) {
  BvBrowserContext* browser_context = static_cast<BvBrowserContext*>(context);
  return content::GeneratedCodeCacheSettings(true, 100 * 1024 * 1024,
                                             browser_context->GetCacheDir());
}

void BvContentBrowserClient::AllowCertificateError(
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    bool is_primary_main_frame_request,
    bool strict_enforcement,
    base::OnceCallback<void(content::CertificateRequestResultType)> callback) {
  BvContentsClientBridge* client =
      BvContentsClientBridge::FromWebContents(web_contents);
  bool cancel_request = true;
  // We only call the callback once but we must pass ownership to a function
  // that conditionally calls it.
  auto split_callback = base::SplitOnceCallback(std::move(callback));
  if (client) {
    client->AllowCertificateError(cert_error, ssl_info.cert.get(), request_url,
                                  std::move(split_callback.first),
                                  &cancel_request);
  }
  if (cancel_request) {
    std::move(split_callback.second)
        .Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
  }
}

base::OnceClosure BvContentBrowserClient::SelectClientCertificate(
    WebContents* web_contents,
    net::SSLCertRequestInfo* cert_request_info,
    net::ClientCertIdentityList client_certs,
    std::unique_ptr<content::ClientCertificateDelegate> delegate) {
  BvContentsClientBridge* client =
      BvContentsClientBridge::FromWebContents(web_contents);
  if (client)
    client->SelectClientCertificate(cert_request_info, std::move(delegate));
  return base::OnceClosure();
}

bool BvContentBrowserClient::CanCreateWindow(
    content::RenderFrameHost* opener,
    const GURL& opener_url,
    const GURL& opener_top_level_frame_url,
    const url::Origin& source_origin,
    content::mojom::WindowContainerType container_type,
    const GURL& target_url,
    const content::Referrer& referrer,
    const std::string& frame_name,
    WindowOpenDisposition disposition,
    const blink::mojom::WindowFeatures& features,
    bool user_gesture,
    bool opener_suppressed,
    bool* no_javascript_access) {
  // We unconditionally allow popup windows at this stage and will give
  // the embedder the opporunity to handle displaying of the popup in
  // WebContentsDelegate::AddContents (via the
  // AwContentsClient.onCreateWindow callback).
  // Note that if the embedder has blocked support for creating popup
  // windows through AwSettings, then we won't get to this point as
  // the popup creation will have been blocked at the WebKit level.
  if (no_javascript_access) {
    *no_javascript_access = false;
  }

  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(opener);
  BvSettings* settings = BvSettings::FromWebContents(web_contents);

  return (settings && settings->GetJavaScriptCanOpenWindowsAutomatically()) ||
         user_gesture;
}

base::FilePath BvContentBrowserClient::GetDefaultDownloadDirectory() {
  // Android WebView does not currently use the Chromium downloads system.
  // Download requests are cancelled immedately when recognized; see
  // AwResourceDispatcherHost::CreateResourceHandlerForDownload. However the
  // download system still tries to start up and calls this before recognizing
  // the request has been cancelled.
  return base::FilePath();
}

std::string BvContentBrowserClient::GetDefaultDownloadName() {
  NOTREACHED() << "BisonView not use chromium downloads";
  return std::string();
}

void BvContentBrowserClient::DidCreatePpapiPlugin(
    content::BrowserPpapiHost* browser_host) {
  NOTREACHED() << "BisonView not support plugins";
}

bool BvContentBrowserClient::AllowPepperSocketAPI(
    content::BrowserContext* browser_context,
    const GURL& url,
    bool private_api,
    const content::SocketPermissionRequest* params) {
  NOTREACHED() << "BisonView does not support plugins";
  return false;
}

bool BvContentBrowserClient::IsPepperVpnProviderAPIAllowed(
    content::BrowserContext* browser_context,
    const GURL& url) {
  NOTREACHED() << "BisonView does not support plugins";
  return false;
}

content::TracingDelegate* BvContentBrowserClient::GetTracingDelegate() {
  return new BvTracingDelegate();
}

void BvContentBrowserClient::GetAdditionalMappedFilesForChildProcess(
    const base::CommandLine& command_line,
    int child_process_id,
    content::PosixFileDescriptorInfo* mappings) {
  base::MemoryMappedFile::Region region;
  int fd = ui::GetMainAndroidPackFd(&region);
  mappings->ShareWithRegion(kBisonViewMainPakDescriptor, fd, region);
  fd = ui::GetCommonResourcesPackFd(&region);
  mappings->ShareWithRegion(kBisonView100PercentPakDescriptor, fd, region);
  fd = ui::GetLocalePackFd(&region);
  mappings->ShareWithRegion(kBisonViewLocalePakDescriptor, fd, region);
  int crash_signal_fd =
      crashpad::CrashHandlerHost::Get()->GetDeathSignalSocket();
  if (crash_signal_fd >= 0) {
    mappings->Share(kCrashDumpSignal, crash_signal_fd);
  }
}

void BvContentBrowserClient::OverrideWebkitPrefs(
    content::WebContents* web_contents,
    blink::web_pref::WebPreferences* web_prefs) {
  BvSettings* bv_settings = BvSettings::FromWebContents(web_contents);
  if (bv_settings) {
    bv_settings->PopulateWebPreferences(web_prefs);
  }
}

std::vector<std::unique_ptr<content::NavigationThrottle>>
BvContentBrowserClient::CreateThrottlesForNavigation(
    content::NavigationHandle* navigation_handle) {
  std::vector<std::unique_ptr<content::NavigationThrottle>> throttles;
  if (navigation_handle->IsInMainFrame()) {
    // MetricsNavigationThrottle requires that it runs before
    // NavigationThrottles that may delay or cancel navigations, so only
    // NavigationThrottles that don't delay or cancel navigations (e.g.
    // throttles that are only observing callbacks without affecting navigation
    // behavior) should be added before MetricsNavigationThrottle.
    throttles.push_back(page_load_metrics::MetricsNavigationThrottle::Create(
        navigation_handle));
    // Use Synchronous mode for the navigation interceptor, since this class
    // doesn't actually call into an arbitrary client, it just posts a task to
    // call onPageStarted. shouldOverrideUrlLoading happens earlier (see
    // ContentBrowserClient::ShouldOverrideUrlLoading).
    std::unique_ptr<content::NavigationThrottle> intercept_navigation_throttle =
        navigation_interception::InterceptNavigationDelegate::
            MaybeCreateThrottleFor(
                navigation_handle,
                navigation_interception::SynchronyMode::kSync);
    if (intercept_navigation_throttle)
      throttles.push_back(std::move(intercept_navigation_throttle));

    throttles.push_back(std::make_unique<PolicyBlocklistNavigationThrottle>(
        navigation_handle, BvBrowserContext::FromWebContents(
                               navigation_handle->GetWebContents())));
  }
  return throttles;
}

std::unique_ptr<content::DevToolsManagerDelegate>
BvContentBrowserClient::CreateDevToolsManagerDelegate() {
  return std::make_unique<BvDevToolsManagerDelegate>();
}

std::vector<std::unique_ptr<blink::URLLoaderThrottle>>
BvContentBrowserClient::CreateURLLoaderThrottles(
    const network::ResourceRequest& request,
    content::BrowserContext* browser_context,
    const base::RepeatingCallback<content::WebContents*()>& wc_getter,
    content::NavigationUIData* navigation_ui_data,
    int frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  std::vector<std::unique_ptr<blink::URLLoaderThrottle>> result;

  if (request.destination == network::mojom::RequestDestination::kDocument) {
    const bool is_load_url =
        request.transition_type & ui::PAGE_TRANSITION_FROM_API;
    const bool is_go_back_forward =
        request.transition_type & ui::PAGE_TRANSITION_FORWARD_BACK;
    const bool is_reload = ui::PageTransitionCoreTypeIs(
        static_cast<ui::PageTransition>(request.transition_type),
        ui::PAGE_TRANSITION_RELOAD);
    if (is_load_url || is_go_back_forward || is_reload) {
      result.push_back(
          std::make_unique<BvURLLoaderThrottle>(static_cast<BvResourceContext*>(
              browser_context->GetResourceContext())));
    }
  }

  return result;
}

static bool IsEnterpriseAuthAppLinkUrl(const GURL& url) {
  PrefService* pref_service =
      bison::BvBrowserProcess::GetInstance()->local_state();

  const base::Value* authentication_url_list =
      pref_service->GetList(prefs::kEnterpriseAuthAppLinkPolicy);

  if (authentication_url_list == nullptr) {
    return false;
  }

  for (const auto& el : authentication_url_list->GetList()) {
    const std::string* policy_url = el.FindStringKey("url");
    GURL authentication_url = GURL(*policy_url);

    // TODO(ayushsha,b/201408457): Use UrlMatcher to match authentication urls.
    if (authentication_url.EqualsIgnoringRef(url)) {
      return true;
    }
  }

  return false;
}

bool BvContentBrowserClient::ShouldOverrideUrlLoading(
    int frame_tree_node_id,
    bool browser_initiated,
    const GURL& gurl,
    const std::string& request_method,
    bool has_user_gesture,
    bool is_redirect,
    bool is_main_frame,
    ui::PageTransition transition,
    bool* ignore_navigation) {
  *ignore_navigation = false;

  if (request_method != "GET")
    return true;

  bool application_initiated =
      browser_initiated || transition & ui::PAGE_TRANSITION_FORWARD_BACK;

  if (application_initiated && !is_redirect)
    return true;

  if (!is_main_frame &&
      (gurl.SchemeIs(url::kHttpScheme) || gurl.SchemeIs(url::kHttpsScheme) ||
       gurl.SchemeIs(url::kAboutScheme)))
    return true;

  WebContents* web_contents =
      WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  if (web_contents == nullptr)
    return true;
  BvContentsClientBridge* client_bridge =
      BvContentsClientBridge::FromWebContents(web_contents);
  if (client_bridge == nullptr)
    return true;

  std::u16string url = base::UTF8ToUTF16(gurl.possibly_invalid_spec());

  BvSettings* aw_settings = BvSettings::FromWebContents(web_contents);
  if ((gurl.SchemeIs(url::kHttpScheme) || gurl.SchemeIs(url::kHttpsScheme)) &&
      aw_settings->enterprise_authentication_app_link_policy_enabled() &&
      IsEnterpriseAuthAppLinkUrl(gurl)) {
    bool success = client_bridge->SendBrowseIntent(url);
    if (success) {
      return true;
    }
  }

  return client_bridge->ShouldOverrideUrlLoading(
      url, has_user_gesture, is_redirect, is_main_frame, ignore_navigation);
}

bool BvContentBrowserClient::
    ShouldIgnoreInitialNavigationEntryNavigationStateChangedForLegacySupport() {
  // On Android WebView, we should not fire the initial NavigationEntry
  // creation/modification NavigationStateChanged calls to preserve legacy
  // behavior (not firing extra onPageFinished calls), as initial
  // NavigationEntries used to not exist. See https://crbug.com/1277414.
  // However, if kWebViewSynthesizePageLoadOnlyOnInitialMainDocumentAccess is
  // enabled, we won't need to ignore the extra NavigationStateChanged() calls,
  // because they won't trigger synthesized page loads and won't cause extra
  // onPageFinished calls.
  return !base::FeatureList::IsEnabled(
      features::kWebViewSynthesizePageLoadOnlyOnInitialMainDocumentAccess);
}

bool BvContentBrowserClient::SupportsAvoidUnnecessaryBeforeUnloadCheckSync() {
  // WebView allows the embedder to override navigation in such a way that
  // might trigger reentrancy if this returned true. See comments in
  // ContentBrowserClient::SupportsAvoidUnnecessaryBeforeUnloadCheckSync() for
  // more details.
  return false;
}

bool BvContentBrowserClient::CreateThreadPool(base::StringPiece name) {
  if (g_should_create_thread_pool) {
    base::ThreadPoolInstance::Create(name);
    return true;
  }
  return false;
}

std::unique_ptr<content::LoginDelegate>
BvContentBrowserClient::CreateLoginDelegate(
    const net::AuthChallengeInfo& auth_info,
    content::WebContents* web_contents,
    const content::GlobalRequestID& request_id,
    bool is_request_for_primary_main_frame,
    const GURL& url,
    scoped_refptr<net::HttpResponseHeaders> response_headers,
    bool first_auth_attempt,
    LoginAuthRequiredCallback auth_required_callback) {
  return std::make_unique<BvHttpAuthHandler>(auth_info, web_contents,
                                             first_auth_attempt,
                                             std::move(auth_required_callback));
}

bool BvContentBrowserClient::HandleExternalProtocol(
    const GURL& url,
    content::WebContents::Getter web_contents_getter,
    int frame_tree_node_id,
    content::NavigationUIData* navigation_data,
    bool is_primary_main_frame,
    bool /* is_in_fenced_frame_tree */,
    network::mojom::WebSandboxFlags /*sandbox_flags*/,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    const absl::optional<url::Origin>& initiating_origin,
    content::RenderFrameHost* initiator_document,
    mojo::PendingRemote<network::mojom::URLLoaderFactory>* out_factory) {
  mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver =
      out_factory->InitWithNewPipeAndPassReceiver();
  // We don't need to care for |security_options| as the factories constructed
  // below are used only for navigation.
  if (content::BrowserThread::CurrentlyOn(content::BrowserThread::IO)) {
    // Manages its own lifetime.
    new bison::BvProxyingURLLoaderFactory(
        frame_tree_node_id, std::move(receiver), mojo::NullRemote(),
        true /* intercept_only */, absl::nullopt /* security_options */);
  } else {
    content::GetIOThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
               int frame_tree_node_id) {
              // Manages its own lifetime.
              new bison::BvProxyingURLLoaderFactory(
                  frame_tree_node_id, std::move(receiver), mojo::NullRemote(),
                  true /* intercept_only */,
                  absl::nullopt /* security_options */);
            },
            std::move(receiver), frame_tree_node_id));
  }
  return false;
}

void BvContentBrowserClient::RegisterNonNetworkSubresourceURLLoaderFactories(
    int render_process_id,
    int render_frame_id,
    const absl::optional<url::Origin>& request_initiator_origin,
    NonNetworkURLLoaderFactoryMap* factories) {
  WebContents* web_contents = content::WebContents::FromRenderFrameHost(
      content::RenderFrameHost::FromID(render_process_id, render_frame_id));
  BvSettings* bv_settings = BvSettings::FromWebContents(web_contents);

  if (bv_settings && bv_settings->GetAllowFileAccess()) {
    BvBrowserContext* bv_browser_context =
        BvBrowserContext::FromWebContents(web_contents);
    factories->emplace(
        url::kFileScheme,
        content::CreateFileURLLoaderFactory(
            bv_browser_context->GetPath(),
            bv_browser_context->GetSharedCorsOriginAccessList()));
  }
}

bool BvContentBrowserClient::ShouldAllowNoLongerUsedProcessToExit() {
  // TODO(crbug.com/1268454): Add Android WebView support for allowing a
  // renderer process to exit when only non-live RenderFrameHosts remain,
  // without consulting the app's OnRenderProcessGone crash handlers.
  return false;
}

bool BvContentBrowserClient::ShouldIsolateErrorPage(bool in_main_frame) {
  return false;
}

bool BvContentBrowserClient::ShouldEnableStrictSiteIsolation() {
  return false;
}

size_t BvContentBrowserClient::GetMaxRendererProcessCountOverride() {
  // TODO(crbug.com/806404): These options can currently can only be turned by
  // by manually overriding command line switches because
  // `ShouldDisableSiteIsolation` returns true. Should coordinate if/when
  // enabling this in production.
  if (content::SiteIsolationPolicy::UseDedicatedProcessesForAllSites() ||
      content::SiteIsolationPolicy::AreIsolatedOriginsEnabled() ||
      content::SiteIsolationPolicy::IsStrictOriginIsolationEnabled()) {
    // Do not restrict the max renderer process count for these site isolation
    // modes. This allows OOPIFs to happen on android webview.
    return 0u;  // Use default.
  }
  return 1u;
}

bool BvContentBrowserClient::ShouldDisableSiteIsolation(
    content::SiteIsolationMode site_isolation_mode) {
  // Since AW does not yet support OOPIFs, we must return true here to disable
  // features that may trigger OOPIFs, such as origin isolation.
  //
  // Adding OOPIF support for AW is tracked by https://crbug.com/806404.
  return true;
}

bool BvContentBrowserClient::ShouldLockProcessToSite(
    content::BrowserContext* browser_context,
    const GURL& effective_url) {
  // TODO(lukasza): https://crbug.cmo/869494: Once Android WebView supports
  // OOPIFs, we should remove this ShouldLockToOrigin overload.  Till then,
  // returning false helps avoid accidentally applying citadel-style Site
  // Isolation enforcement to Android WebView (and causing incorrect renderer
  // kills).
  return false;
}

bool BvContentBrowserClient::WillCreateURLLoaderFactory(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* frame,
    int render_process_id,
    URLLoaderFactoryType type,
    const url::Origin& request_initiator,
    absl::optional<int64_t> navigation_id,
    ukm::SourceIdObj ukm_source_id,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory>* factory_receiver,
    mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
        header_client,
    bool* bypass_redirect_checks,
    bool* disable_secure_dns,
    network::mojom::URLLoaderFactoryOverridePtr* factory_override) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  mojo::PendingReceiver<network::mojom::URLLoaderFactory> proxied_receiver;
  mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory_remote;

  if (factory_override) {
    // We are interested in factories "inside" of CORS, so use
    // |factory_override|.
    *factory_override = network::mojom::URLLoaderFactoryOverride::New();
    proxied_receiver =
        (*factory_override)
            ->overriding_factory.InitWithNewPipeAndPassReceiver();
    (*factory_override)->overridden_factory_receiver =
        target_factory_remote.InitWithNewPipeAndPassReceiver();
    (*factory_override)->skip_cors_enabled_scheme_check = true;
  } else {
    // In this case, |factory_override| is not given. But all callers of
    // ContentBrowserClient::WillCreateURLLoaderFactory guarantee that
    // |factory_override| is null only when the security features on the network
    // service is no-op for requests coming to the URLLoaderFactory. Hence we
    // can use |factory_receiver| here.
    proxied_receiver = std::move(*factory_receiver);
    *factory_receiver = target_factory_remote.InitWithNewPipeAndPassReceiver();
  }
  // Android WebView has one non off-the-record browser context.
  if (frame) {
    auto security_options =
        absl::make_optional<BvProxyingURLLoaderFactory::SecurityOptions>();
    security_options->disable_web_security =
        base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kDisableWebSecurity);
    const auto& preferences =
        WebContents::FromRenderFrameHost(frame)->GetOrCreateWebPreferences();
    // See also //android_webview/docs/cors-and-webview-api.md to understand how
    // each settings affect CORS behaviors on file:// and content://.
    if (request_initiator.scheme() == url::kFileScheme) {
      security_options->disable_web_security |=
          preferences.allow_universal_access_from_file_urls;
      // Usual file:// to file:// requests are mapped to kNoCors if the setting
      // is set to true. Howover, file:///android_{asset|res}/ still uses kCors
      // and needs to permit it in the |security_options|.
      security_options->allow_cors_to_same_scheme =
          preferences.allow_file_access_from_file_urls;
    } else if (request_initiator.scheme() == url::kContentScheme) {
      security_options->allow_cors_to_same_scheme =
          preferences.allow_file_access_from_file_urls ||
          preferences.allow_universal_access_from_file_urls;
    }

    content::GetIOThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(&BvProxyingURLLoaderFactory::CreateProxy,
                       frame->GetFrameTreeNodeId(), std::move(proxied_receiver),
                       std::move(target_factory_remote), security_options));
  } else {
    // A service worker and worker subresources set nullptr to |frame|, and
    // work without seeing the AllowUniversalAccessFromFileURLs setting. So,
    // we don't pass a valid |security_options| here.
    content::GetIOThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(&BvProxyingURLLoaderFactory::CreateProxy,
                                  content::RenderFrameHost::kNoFrameTreeNodeId,
                                  std::move(proxied_receiver),
                                  std::move(target_factory_remote),
                                  absl::nullopt /* security_options */));
  }
  return true;
}

uint32_t BvContentBrowserClient::GetWebSocketOptions(
    content::RenderFrameHost* frame) {
  uint32_t options = network::mojom::kWebSocketOptionNone;
  if (!frame) {
    return options;
  }
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(frame);
  BvContents* bison_contents = BvContents::FromWebContents(web_contents);

  bool global_cookie_policy =
      BvCookieAccessPolicy::GetInstance()->GetShouldAcceptCookies();
  bool third_party_cookie_policy = bison_contents->AllowThirdPartyCookies();
  if (!global_cookie_policy) {
    options |= network::mojom::kWebSocketOptionBlockAllCookies;
  } else if (!third_party_cookie_policy) {
    options |= network::mojom::kWebSocketOptionBlockThirdPartyCookies;
  }
  return options;
}

bool BvContentBrowserClient::WillCreateRestrictedCookieManager(
    network::mojom::RestrictedCookieManagerRole role,
    content::BrowserContext* browser_context,
    const url::Origin& origin,
    const net::IsolationInfo& isolation_info,
    bool is_service_worker,
    int process_id,
    int routing_id,
    mojo::PendingReceiver<network::mojom::RestrictedCookieManager>* receiver) {
  mojo::PendingReceiver<network::mojom::RestrictedCookieManager> orig_receiver =
      std::move(*receiver);

  mojo::PendingRemote<network::mojom::RestrictedCookieManager>
      target_rcm_remote;
  *receiver = target_rcm_remote.InitWithNewPipeAndPassReceiver();

  BisonProxyingRestrictedCookieManager::CreateAndBind(
      std::move(target_rcm_remote), is_service_worker, process_id, routing_id,
      std::move(orig_receiver));

  return false;  // only made a proxy, still need the actual impl to be made.
}

std::string BvContentBrowserClient::GetProduct() {
  return bison::GetProduct();
}

std::string BvContentBrowserClient::GetUserAgent() {
  return bison::GetUserAgent();
}

content::ContentBrowserClient::WideColorGamutHeuristic
BvContentBrowserClient::GetWideColorGamutHeuristic() {
  if (base::FeatureList::IsEnabled(features::kWebViewWideColorGamutSupport))
    return WideColorGamutHeuristic::kUseWindow;

  if (display::HasForceDisplayColorProfile() &&
      display::GetForcedDisplayColorProfile() ==
          gfx::ColorSpace::CreateDisplayP3D65()) {
    return WideColorGamutHeuristic::kUseWindow;
  }

  return WideColorGamutHeuristic::kNone;
}

void BvContentBrowserClient::LogWebFeatureForCurrentPage(
    content::RenderFrameHost* render_frame_host,
    blink::mojom::WebFeature feature) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  page_load_metrics::MetricsWebContentsObserver::RecordFeatureUsage(
      render_frame_host, feature);
}

bool BvContentBrowserClient::ShouldAllowInsecurePrivateNetworkRequests(
    content::BrowserContext* browser_context,
    const url::Origin& origin) {
  // Webview does not implement support for deprecation trials, so webview apps
  // broken by Private Network Access restrictions cannot help themselves by
  // registering for the trial.
  // See crbug.com/1255675.
  return true;
}

content::SpeechRecognitionManagerDelegate*
BvContentBrowserClient::CreateSpeechRecognitionManagerDelegate() {
  return nullptr;
  //jiang
  //return new content::SpeechRecognitionManagerDelegate();
}

bool BvContentBrowserClient::HasErrorPage(int http_status_code) {
  return http_status_code >= 400;
}

bool BvContentBrowserClient::SuppressDifferentOriginSubframeJSDialogs(
    content::BrowserContext* browser_context) {
  return base::FeatureList::IsEnabled(
      features::kWebViewSuppressDifferentOriginSubframeJSDialogs);
}

// static
void BvContentBrowserClient::DisableCreatingThreadPool() {
  g_should_create_thread_pool = false;
}

}  // namespace bison
