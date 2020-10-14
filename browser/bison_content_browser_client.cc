#include "bison/browser/bison_content_browser_client.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "bison/browser/bison_browser_context.h"
#include "bison/browser/bison_browser_main_parts.h"
#include "bison/browser/bison_content_browser_overlay_manifest.h"
#include "bison/browser/bison_content_renderer_overlay_manifest.h"
#include "bison/browser/bison_contents.h"
#include "bison/browser/bison_contents_client_bridge.h"
#include "bison/browser/bison_contents_io_thread_client.h"
#include "bison/browser/bison_cookie_access_policy.h"
#include "bison/browser/bison_devtools_manager_delegate.h"
#include "bison/browser/bison_feature_list_creator.h"
#include "bison/browser/bison_settings.h"
#include "bison/browser/cookie_manager.h"
#include "bison/browser/network_service/bison_proxying_restricted_cookie_manager.h"
#include "bison/browser/network_service/bison_proxying_url_loader_factory.h"
#include "bison/browser/network_service/bison_url_loader_throttle.h"
#include "bison/common/bison_descriptors.h"
#include "bison/common/render_view_messages.h"
#include "bison/common/url_constants.h"

#include "base/android/apk_assets.h"
#include "base/android/locale_utils.h"
#include "base/base_paths_android.h"
#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/debug/leak_annotations.h"
#include "base/feature_list.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "build/build_config.h"
#include "components/cdm/browser/cdm_message_filter_android.h"
#include "components/cdm/browser/media_drm_storage_impl.h"
#include "components/crash/content/browser/crash_handler_host_linux.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#include "components/page_load_metrics/browser/metrics_navigation_throttle.h"
#include "components/page_load_metrics/browser/metrics_web_contents_observer.h"
#include "components/policy/content/policy_blacklist_navigation_throttle.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_task_traits.h"  //
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "content/public/browser/cors_exempt_headers.h"
#include "content/public/browser/login_delegate.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_descriptors.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/user_agent.h"
#include "content/public/common/web_preferences.h"
#include "media/mojo/buildflags.h"
#include "net/ssl/client_cert_identity.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/service_manager/public/cpp/manifest.h"
#include "services/service_manager/public/cpp/manifest_builder.h"
#include "storage/browser/quota/quota_settings.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/ui_base_switches.h"
#include "url/gurl.h"
#include "url/origin.h"

#if BUILDFLAG(ENABLE_MOJO_MEDIA_IN_BROWSER_PROCESS) || \
    BUILDFLAG(ENABLE_CAST_RENDERER)
#include "media/mojo/mojom/constants.mojom.h"           // nogncheck
#include "media/mojo/services/media_service_factory.h"  // nogncheck
#endif

using content::BrowserThread;
using content::StoragePartition;
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

class BisonContentsMessageFilter : public content::BrowserMessageFilter {
 public:
  explicit BisonContentsMessageFilter(int process_id);

  // BrowserMessageFilter methods.
  void OverrideThreadForMessage(const IPC::Message& message,
                                BrowserThread::ID* thread) override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnShouldOverrideUrlLoading(int routing_id,
                                  const base::string16& url,
                                  bool has_user_gesture,
                                  bool is_redirect,
                                  bool is_main_frame,
                                  bool* ignore_navigation);
  void OnSubFrameCreated(int parent_render_frame_id, int child_render_frame_id);

 private:
  ~BisonContentsMessageFilter() override;

  int process_id_;

  DISALLOW_COPY_AND_ASSIGN(BisonContentsMessageFilter);
};

BisonContentsMessageFilter::BisonContentsMessageFilter(int process_id)
    : BrowserMessageFilter(BisonViewMsgStart), process_id_(process_id) {}

BisonContentsMessageFilter::~BisonContentsMessageFilter() = default;

void BisonContentsMessageFilter::OverrideThreadForMessage(
    const IPC::Message& message,
    BrowserThread::ID* thread) {
  if (message.type() == BisonViewHostMsg_ShouldOverrideUrlLoading::ID) {
    *thread = BrowserThread::UI;
  }
}

bool BisonContentsMessageFilter::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(BisonContentsMessageFilter, message)
    IPC_MESSAGE_HANDLER(BisonViewHostMsg_ShouldOverrideUrlLoading,
                        OnShouldOverrideUrlLoading)
    IPC_MESSAGE_HANDLER(BisonViewHostMsg_SubFrameCreated, OnSubFrameCreated)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void BisonContentsMessageFilter::OnShouldOverrideUrlLoading(
    int render_frame_id,
    const base::string16& url,
    bool has_user_gesture,
    bool is_redirect,
    bool is_main_frame,
    bool* ignore_navigation) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  VLOG(0) << "OnShouldOverrideUrlLoading";
  *ignore_navigation = false;
  BisonContentsClientBridge* client =
      BisonContentsClientBridge::FromID(process_id_, render_frame_id);
  if (client) {
    if (!client->ShouldOverrideUrlLoading(url, has_user_gesture, is_redirect,
                                          is_main_frame, ignore_navigation)) {
      // If the shouldOverrideUrlLoading call caused a java exception we should
      // always return immediately here!
      return;
    }
  } else {
    LOG(WARNING) << "Failed to find the associated render view host for url: "
                 << url;
  }
}

void BisonContentsMessageFilter::OnSubFrameCreated(int parent_render_frame_id,
                                                   int child_render_frame_id) {
  BisonContentsIoThreadClient::SubFrameCreated(
      process_id_, parent_render_frame_id, child_render_frame_id);
}

// A dummy binder for mojo interface autofill::mojom::PasswordManagerDriver.
// void DummyBindPasswordManagerDriver(
//     mojo::PendingReceiver<autofill::mojom::PasswordManagerDriver> receiver,
//     content::RenderFrameHost* render_frame_host) {}

void PassMojoCookieManagerToCookieManager(
    CookieManager* cookie_manager,
    const mojo::Remote<network::mojom::NetworkContext>& network_context) {
  // Get the CookieManager from the NetworkContext.
  mojo::PendingRemote<network::mojom::CookieManager> cookie_manager_remote;
  network_context->GetCookieManager(
      cookie_manager_remote.InitWithNewPipeAndPassReceiver());

  // Pass the mojo::PendingRemote<network::mojom::CookieManager> to
  // bison::CookieManager, so it can implement its APIs with this mojo
  // CookieManager.
  cookie_manager->SetMojoCookieManager(std::move(cookie_manager_remote));
}

#if BUILDFLAG(ENABLE_MOJO_CDM)
void CreateOriginId(cdm::MediaDrmStorageImpl::OriginIdObtainedCB callback) {
  std::move(callback).Run(true, base::UnguessableToken::Create());
}

void AllowEmptyOriginIdCB(base::OnceCallback<void(bool)> callback) {
  // Since CreateOriginId() always returns a non-empty origin ID, we don't need
  // to allow empty origin ID.
  std::move(callback).Run(false);
}

void CreateMediaDrmStorage(content::RenderFrameHost* render_frame_host,
                           ::media::mojom::MediaDrmStorageRequest request) {
  DCHECK(render_frame_host);

  if (render_frame_host->GetLastCommittedOrigin().opaque()) {
    DVLOG(1) << __func__ << ": Unique origin.";
    return;
  }

  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  DCHECK(web_contents) << "WebContents not available.";

  auto* bison_browser_context =
      static_cast<BisonBrowserContext*>(web_contents->GetBrowserContext());
  DCHECK(bison_browser_context) << "BisonBrowserContext not available.";

  PrefService* pref_service = bison_browser_context->GetPrefService();
  DCHECK(pref_service);

  // The object will be deleted on connection error, or when the frame navigates
  // away.
  new cdm::MediaDrmStorageImpl(
      render_frame_host, pref_service, base::BindRepeating(&CreateOriginId),
      base::BindRepeating(&AllowEmptyOriginIdCB), std::move(request));
}
#endif  // BUILDFLAG(ENABLE_MOJO_CDM)

}  // namespace

std::string GetProduct() {
  return "Chromium/" + version_info::GetVersionNumber();
}

std::string GetUserAgent() {
  std::string product = "Bison/1.0 " + GetProduct();
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kUseMobileUserAgent))
    product += " Mobile";
  return content::BuildUserAgentFromProductAndExtraOSInfo(product, "; wv",
                                                          true);
}

// TODO(yirui): can use similar logic as in PrependToAcceptLanguagesIfNecessary
// in chrome/browser/android/preferences/pref_service_bridge.cc
// static
std::string BisonContentBrowserClient::GetAcceptLangsImpl() {
  // Start with the current locale(s) in BCP47 format.
  std::string locales_string = BisonContents::GetLocaleList();

  // If accept languages do not contain en-US, add in en-US which will be
  // used with a lower q-value.
  if (locales_string.find("en-US") == std::string::npos)
    locales_string += ",en-US";
  return locales_string;
}

// static
void BisonContentBrowserClient::set_check_cleartext_permitted(bool permitted) {
#if DCHECK_IS_ON()
  DCHECK(!g_created_network_context_params);
#endif
  g_check_cleartext_permitted = permitted;
}

bool BisonContentBrowserClient::get_check_cleartext_permitted() {
  return g_check_cleartext_permitted;
}

BisonContentBrowserClient::BisonContentBrowserClient(
    BisonFeatureListCreator* feature_list_creator)
    : bison_feature_list_creator_(feature_list_creator) {
  DCHECK(bison_feature_list_creator_);

  // frame_interfaces_.AddInterface(
  //     base::BindRepeating(&DummyBindPasswordManagerDriver));
}

BisonContentBrowserClient::~BisonContentBrowserClient() {}

void BisonContentBrowserClient::OnNetworkServiceCreated(
    network::mojom::NetworkService* network_service) {
  network::mojom::HttpAuthStaticParamsPtr auth_static_params =
      network::mojom::HttpAuthStaticParams::New();
  auth_static_params->supported_schemes = BisonBrowserContext::GetAuthSchemes();
  content::GetNetworkService()->SetUpHttpAuth(std::move(auth_static_params));
}

mojo::Remote<network::mojom::NetworkContext>
BisonContentBrowserClient::CreateNetworkContext(
    BrowserContext* context,
    bool in_memory,
    const base::FilePath& relative_partition_path) {
  DCHECK(context);

  auto* bison_context = static_cast<BisonBrowserContext*>(context);
  mojo::Remote<network::mojom::NetworkContext> network_context;
  network::mojom::NetworkContextParamsPtr context_params =
      bison_context->GetNetworkContextParams(in_memory,
                                             relative_partition_path);

#if DCHECK_IS_ON()
  g_created_network_context_params = true;
#endif
  content::GetNetworkService()->CreateNetworkContext(
      network_context.BindNewPipeAndPassReceiver(), std::move(context_params));

  // Pass a CookieManager to the code supporting BisonCookieManager.java (i.e.,
  // the Cookies APIs).
  PassMojoCookieManagerToCookieManager(bison_context->GetCookieManager(),
                                       network_context);
  return network_context;
}

BisonBrowserContext* BisonContentBrowserClient::InitBrowserContext() {
  browser_context_ = std::make_unique<BisonBrowserContext>();
  return browser_context_.get();
}

std::unique_ptr<BrowserMainParts>
BisonContentBrowserClient::CreateBrowserMainParts(
    const MainFunctionParams& parameters) {
  return std::make_unique<BisonBrowserMainParts>(this);
}

void BisonContentBrowserClient::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  // Grant content: scheme access to the whole renderer process, since weimpose
  // per-view access checks, and access is granted by default (see
  // BisonSettings.mAllowContentUrlAccess).
  content::ChildProcessSecurityPolicy::GetInstance()->GrantRequestScheme(
      host->GetID(), url::kContentScheme);

  host->AddFilter(new BisonContentsMessageFilter(host->GetID()));
  // WebView always allows persisting data.
  host->AddFilter(new cdm::CdmMessageFilterAndroid(true, false));
}

bool BisonContentBrowserClient::IsExplicitNavigation(
    ui::PageTransition transition) {
  return ui::PageTransitionCoreTypeIs(transition, ui::PAGE_TRANSITION_TYPED);
}

bool BisonContentBrowserClient::ShouldUseMobileFlingCurve() {
  return true;
}

bool BisonContentBrowserClient::IsHandledURL(const GURL& url) {
  if (!url.is_valid())
    return true;
  const std::string scheme = url.scheme();
  DCHECK_EQ(scheme, base::ToLowerASCII(scheme));
  static const char* const kProtocolList[] = {
      url::kDataScheme,         url::kBlobScheme,    url::kFileSystemScheme,
      content::kChromeUIScheme, url::kContentScheme,
  };
  if (scheme == url::kFileScheme) {
    // Return false for the "special" file URLs, so they can be loaded
    // even if access to file: scheme is not granted to the child process.
    return !IsAndroidSpecialFileUrl(url);
  }
  for (size_t i = 0; i < base::size(kProtocolList); ++i) {
    if (scheme == kProtocolList[i])
      return true;
  }
  return net::URLRequest::IsHandledProtocol(scheme);
}

void BisonContentBrowserClient::BindInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  if (!frame_interfaces_) {
    frame_interfaces_ = std::make_unique<
        service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>>();
  }

  frame_interfaces_->TryBindInterface(interface_name, &interface_pipe,
                                      render_frame_host);
}

void BisonContentBrowserClient::RunServiceInstance(
    const service_manager::Identity& identity,
    mojo::PendingReceiver<service_manager::mojom::Service>* receiver) {
#if BUILDFLAG(ENABLE_MOJO_MEDIA_IN_BROWSER_PROCESS) || \
    BUILDFLAG(ENABLE_CAST_RENDERER)
  bool is_media_service = false;
#if BUILDFLAG(ENABLE_MOJO_MEDIA_IN_BROWSER_PROCESS)
  if (identity.name() == media::mojom::kMediaServiceName)
    is_media_service = true;
#endif  // BUILDFLAG(ENABLE_MOJO_MEDIA_IN_BROWSER_PROCESS)
#if BUILDFLAG(ENABLE_CAST_RENDERER)
  if (identity.name() == media::mojom::kMediaRendererServiceName)
    is_media_service = true;
#endif  // BUILDFLAG(ENABLE_CAST_RENDERER)

    // if (is_media_service) {
    //   service_manager::Service::RunAsyncUntilTermination(
    //       media::CreateMediaServiceForTesting(std::move(*receiver)));
    // }
#endif  // BUILDFLAG(ENABLE_MOJO_MEDIA_IN_BROWSER_PROCESS) ||
        // BUILDFLAG(ENABLE_CAST_RENDERER)
}

bool BisonContentBrowserClient::ShouldTerminateOnServiceQuit(
    const service_manager::Identity& id) {
  if (should_terminate_on_service_quit_callback_)
    return std::move(should_terminate_on_service_quit_callback_).Run(id);
  return false;
}

base::Optional<service_manager::Manifest>
BisonContentBrowserClient::GetServiceManifestOverlay(base::StringPiece name) {
  // Check failed: false. The Service Manager prevented service
  // "content_renderer" from binding interface "ws.mojom.Gpu" in target service
  // "content_browser". You probably need to update one or more service
  // manifests to ensure that "content_browser" exposes "ws.mojom.Gpu" through a
  // capability
  // and that "content_renderer" requires that capability from the
  // "content_browser" service.
  // VLOG(0) << "GetServiceManifestOverlay : name " << name;
  if (name == content::mojom::kBrowserServiceName)
    return GetContentBrowserOverlayManifest();
  if (name == content::mojom::kRendererServiceName)
    return GetContentRendererOverlayManifest();
  return base::nullopt;
}

void BisonContentBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int child_process_id) {
  // if (base::CommandLine::ForCurrentProcess()->HasSwitch(
  //         switches::kExposeInternalsForTesting)) {
  //   command_line->AppendSwitch(switches::kExposeInternalsForTesting);
  // }
  // if (base::CommandLine::ForCurrentProcess()->HasSwitch(
  //         switches::kEnableCrashReporter)) {
  //   command_line->AppendSwitch(switches::kEnableCrashReporter);
  // }
  // if (base::CommandLine::ForCurrentProcess()->HasSwitch(
  //         switches::kCrashDumpsDir)) {
  //   command_line->AppendSwitchPath(
  //       switches::kCrashDumpsDir,
  //       base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(
  //           switches::kCrashDumpsDir));
  // }
  // if (base::CommandLine::ForCurrentProcess()->HasSwitch(
  //         switches::kRegisterFontFiles)) {
  //   command_line->AppendSwitchASCII(
  //       switches::kRegisterFontFiles,
  //       base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
  //           switches::kRegisterFontFiles));
  // }
}

std::string BisonContentBrowserClient::GetAcceptLangs(BrowserContext* context) {
  return GetAcceptLangsImpl();
}

std::string BisonContentBrowserClient::GetDefaultDownloadName() {
  NOTREACHED() << "BisonView does not use chromium downloads";
  return std::string();
}

bool BisonContentBrowserClient::AllowAppCache(
    const GURL& manifest_url,
    const GURL& first_party,
    content::BrowserContext* context) {
  return true;
}

WebContentsViewDelegate* BisonContentBrowserClient::GetWebContentsViewDelegate(
    WebContents* web_contents) {
  // return CreateShellWebContentsViewDelegate(web_contents);
  return nullptr;
}

// scoped_refptr<content::QuotaPermissionContext>
// BisonContentBrowserClient::CreateQuotaPermissionContext() {
//   return new ShellQuotaPermissionContext();
// }

void BisonContentBrowserClient::GetQuotaSettings(
    BrowserContext* context,
    StoragePartition* partition,
    storage::OptionalQuotaSettingsCallback callback) {
  std::move(callback).Run(storage::GetHardCodedSettings(100 * 1024 * 1024));
}

GeneratedCodeCacheSettings
BisonContentBrowserClient::GetGeneratedCodeCacheSettings(
    content::BrowserContext* context) {
  // If we pass 0 for size, disk_cache will pick a default size using the
  // heuristics based on available disk size. These are implemented in
  // disk_cache::PreferredCacheSize in net/disk_cache/cache_util.cc.
  BisonBrowserContext* browser_context =
      static_cast<BisonBrowserContext*>(context);
  return GeneratedCodeCacheSettings(true, 0, browser_context->GetCacheDir());
}

void BisonContentBrowserClient::AllowCertificateError(
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    bool is_main_frame_request,
    bool strict_enforcement,
    const base::Callback<void(content::CertificateRequestResultType)>&
        callback) {
  BisonContentsClientBridge* client =
      BisonContentsClientBridge::FromWebContents(web_contents);
  bool cancel_request = true;
  if (client)
    client->AllowCertificateError(cert_error, ssl_info.cert.get(), request_url,
                                  callback, &cancel_request);
  if (cancel_request)
    callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
}

base::OnceClosure BisonContentBrowserClient::SelectClientCertificate(
    WebContents* web_contents,
    net::SSLCertRequestInfo* cert_request_info,
    net::ClientCertIdentityList client_certs,
    std::unique_ptr<ClientCertificateDelegate> delegate) {
  BisonContentsClientBridge* client =
      BisonContentsClientBridge::FromWebContents(web_contents);
  if (client)
    client->SelectClientCertificate(cert_request_info, std::move(delegate));
  return base::OnceClosure();
}

void BisonContentBrowserClient::GetAdditionalMappedFilesForChildProcess(
    const base::CommandLine& command_line,
    int child_process_id,
    content::PosixFileDescriptorInfo* mappings) {
  mappings->ShareWithRegion(
      kBisonPakDescriptor,
      base::GlobalDescriptors::GetInstance()->Get(kBisonPakDescriptor),
      base::GlobalDescriptors::GetInstance()->GetRegion(kBisonPakDescriptor));

  int crash_signal_fd =
      crashpad::CrashHandlerHost::Get()->GetDeathSignalSocket();
  if (crash_signal_fd >= 0) {
    mappings->Share(service_manager::kCrashDumpSignal, crash_signal_fd);
  }
}

void BisonContentBrowserClient::OverrideWebkitPrefs(
    RenderViewHost* render_view_host,
    WebPreferences* prefs) {
  BisonSettings* bison_settings = BisonSettings::FromWebContents(
      content::WebContents::FromRenderViewHost(render_view_host));
  if (bison_settings) {
    bison_settings->PopulateWebPreferences(prefs);
  }
}

std::vector<std::unique_ptr<content::NavigationThrottle>>
BisonContentBrowserClient::CreateThrottlesForNavigation(
    content::NavigationHandle* navigation_handle) {
  VLOG(0) << "CreateThrottlesForNavigation";
  std::vector<std::unique_ptr<content::NavigationThrottle>> throttles;
  if (navigation_handle->IsInMainFrame()) {
    // 这个好像可以不加
    throttles.push_back(page_load_metrics::MetricsNavigationThrottle::Create(
        navigation_handle));
    throttles.push_back(
        navigation_interception::InterceptNavigationDelegate::CreateThrottleFor(
            navigation_handle, navigation_interception::SynchronyMode::kSync));
    // 这个好像可以不加
    throttles.push_back(std::make_unique<PolicyBlacklistNavigationThrottle>(
        navigation_handle, BisonBrowserContext::FromWebContents(
                               navigation_handle->GetWebContents())));
  }
  return throttles;
}

DevToolsManagerDelegate*
BisonContentBrowserClient::GetDevToolsManagerDelegate() {
  return new BisonDevToolsManagerDelegate();
}

std::vector<std::unique_ptr<blink::URLLoaderThrottle>>
BisonContentBrowserClient::CreateURLLoaderThrottles(
    const network::ResourceRequest& request,
    content::BrowserContext* browser_context,
    const base::RepeatingCallback<content::WebContents*()>& wc_getter,
    content::NavigationUIData* navigation_ui_data,
    int frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  std::vector<std::unique_ptr<blink::URLLoaderThrottle>> result;

  if (request.resource_type ==
      static_cast<int>(content::ResourceType::kMainFrame)) {
    const bool is_load_url =
        request.transition_type & ui::PAGE_TRANSITION_FROM_API;
    const bool is_go_back_forward =
        request.transition_type & ui::PAGE_TRANSITION_FORWARD_BACK;
    const bool is_reload = ui::PageTransitionCoreTypeIs(
        static_cast<ui::PageTransition>(request.transition_type),
        ui::PAGE_TRANSITION_RELOAD);
    if (is_load_url || is_go_back_forward || is_reload) {
      result.push_back(std::make_unique<BisonURLLoaderThrottle>(
          static_cast<BisonResourceContext*>(
              browser_context->GetResourceContext())));
    }
  }

  return result;
}

void BisonContentBrowserClient::ExposeInterfacesToMediaService(
    service_manager::BinderRegistry* registry,
    content::RenderFrameHost* render_frame_host) {
#if BUILDFLAG(ENABLE_MOJO_CDM)
  registry->AddInterface(
      base::BindRepeating(&CreateMediaDrmStorage, render_frame_host));
#endif
}

bool BisonContentBrowserClient::ShouldOverrideUrlLoading(
    int frame_tree_node_id,
    bool browser_initiated,
    const GURL& gurl,
    const std::string& request_method,
    bool has_user_gesture,
    bool is_redirect,
    bool is_main_frame,
    ui::PageTransition transition,
    bool* ignore_navigation) {
  VLOG(0) << "ShouldOverrideUrlLoading";
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

  BisonContentsClientBridge* client_bridge =
      BisonContentsClientBridge::FromWebContents(web_contents);
  if (client_bridge == nullptr)
    return true;

  base::string16 url = base::UTF8ToUTF16(gurl.possibly_invalid_spec());
  return client_bridge->ShouldOverrideUrlLoading(
      url, has_user_gesture, is_redirect, is_main_frame, ignore_navigation);
}

bool BisonContentBrowserClient::ShouldCreateThreadPool() {
  return g_should_create_thread_pool;
}

std::unique_ptr<LoginDelegate> BisonContentBrowserClient::CreateLoginDelegate(
    const net::AuthChallengeInfo& auth_info,
    content::WebContents* web_contents,
    const content::GlobalRequestID& request_id,
    bool is_main_frame,
    const GURL& url,
    scoped_refptr<net::HttpResponseHeaders> response_headers,
    bool first_auth_attempt,
    LoginAuthRequiredCallback auth_required_callback) {
  return nullptr;
}

bool BisonContentBrowserClient::ShouldIsolateErrorPage(bool in_main_frame) {
  return false;
}

bool BisonContentBrowserClient::ShouldEnableStrictSiteIsolation() {
  // TODO(lukasza): When/if we eventually add OOPIF support for AW we should
  // consider running AW tests with and without site-per-process (and this might
  // require returning true below).  Adding OOPIF support for AW is tracked by
  // https://crbug.com/869494.
  return false;
}

bool BisonContentBrowserClient::ShouldLockToOrigin(
    content::BrowserContext* browser_context,
    const GURL& effective_url) {
  // TODO(lukasza): https://crbug.cmo/869494: Once Android WebView supports
  // OOPIFs, we should remove this ShouldLockToOrigin overload.  Till then,
  // returning false helps avoid accidentally applying citadel-style Site
  // Isolation enforcement to Android WebView (and causing incorrect renderer
  // kills).
  return false;
}

bool BisonContentBrowserClient::WillCreateURLLoaderFactory(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* frame,
    int render_process_id,
    URLLoaderFactoryType type,
    const url::Origin& request_initiator,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory>* factory_receiver,
    mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
        header_client,
    bool* bypass_redirect_checks) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto proxied_receiver = std::move(*factory_receiver);
  network::mojom::URLLoaderFactoryPtrInfo target_factory_info;
  *factory_receiver = mojo::MakeRequest(&target_factory_info);
  int process_id =
      type == URLLoaderFactoryType::kNavigation ? 0 : render_process_id;

  // BisonView has one non off-the-record browser context.
  base::PostTask(FROM_HERE, {content::BrowserThread::IO},
                 base::BindOnce(&BisonProxyingURLLoaderFactory::CreateProxy,
                                process_id, std::move(proxied_receiver),
                                std::move(target_factory_info)));
  return true;
}

void BisonContentBrowserClient::
    WillCreateURLLoaderFactoryForAppCacheSubresource(
        int render_process_id,
        mojo::PendingRemote<network::mojom::URLLoaderFactory>*
            pending_factory) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto pending_proxy = std::move(*pending_factory);
  mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver =
      pending_factory->InitWithNewPipeAndPassReceiver();

  base::PostTask(FROM_HERE, {content::BrowserThread::IO},
                 base::BindOnce(&BisonProxyingURLLoaderFactory::CreateProxy,
                                render_process_id, std::move(factory_receiver),
                                std::move(pending_proxy)));
}

uint32_t BisonContentBrowserClient::GetWebSocketOptions(
    content::RenderFrameHost* frame) {
  uint32_t options = network::mojom::kWebSocketOptionNone;
  if (!frame) {
    return options;
  }
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(frame);
  BisonContents* bison_contents = BisonContents::FromWebContents(web_contents);

  bool global_cookie_policy =
      BisonCookieAccessPolicy::GetInstance()->GetShouldAcceptCookies();
  bool third_party_cookie_policy = bison_contents->AllowThirdPartyCookies();
  if (!global_cookie_policy) {
    options |= network::mojom::kWebSocketOptionBlockAllCookies;
  } else if (!third_party_cookie_policy) {
    options |= network::mojom::kWebSocketOptionBlockThirdPartyCookies;
  }
  return options;
}

bool BisonContentBrowserClient::WillCreateRestrictedCookieManager(
    network::mojom::RestrictedCookieManagerRole role,
    content::BrowserContext* browser_context,
    const url::Origin& origin,
    const GURL& site_for_cookies,
    const url::Origin& top_frame_origin,
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

std::string BisonContentBrowserClient::GetProduct() {
  return bison::GetProduct();
}

std::string BisonContentBrowserClient::GetUserAgent() {
  return bison::GetUserAgent();
}

void BisonContentBrowserClient::LogWebFeatureForCurrentPage(
    content::RenderFrameHost* render_frame_host,
    blink::mojom::WebFeature feature) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  page_load_metrics::mojom::PageLoadFeatures new_features({feature}, {}, {});
  page_load_metrics::MetricsWebContentsObserver::RecordFeatureUsage(
      render_frame_host, new_features);
}

// content::SpeechRecognitionManagerDelegate*
// BisonContentBrowserClient::CreateSpeechRecognitionManagerDelegate() {
//   return new BisonSpeechRecognitionManagerDelegate();
// }

// static
void BisonContentBrowserClient::DisableCreatingThreadPool() {
  g_should_create_thread_pool = false;
}

}  // namespace bison
