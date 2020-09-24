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
#include "bison/browser/bison_devtools_manager_delegate.h"
#include "bison/browser/bison_settings.h"
#include "bison/browser/cookie_manager.h"
#include "bison/browser/network_service/bison_proxying_url_loader_factory.h"
#include "bison/common/bison_descriptors.h"
#include "bison/common/render_view_messages.h"

#include "base/android/apk_assets.h"
#include "base/android/locale_utils.h"
#include "base/android/path_utils.h"
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
#include "components/crash/content/browser/crash_handler_host_linux.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#include "components/page_load_metrics/browser/metrics_navigation_throttle.h"
#include "components/page_load_metrics/browser/metrics_web_contents_observer.h"
#include "components/policy/content/policy_blacklist_navigation_throttle.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_task_traits.h"  //
#include "content/public/browser/browser_thread.h"
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
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"
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

#if DCHECK_IS_ON()
// A boolean value to determine if the NetworkContext has been created yet. This
// exists only to check correctness: g_check_cleartext_permitted may only be set
// before the NetworkContext has been created (otherwise,
// g_check_cleartext_permitted won't have any effect).
bool g_created_network_context_params = false;
#endif

BisonContentBrowserClient* g_browser_client;
// On apps targeting API level O or later, check cleartext is enforced.
bool g_check_cleartext_permitted = false;

// #if defined(OS_ANDROID)
// int GetCrashSignalFD(const base::CommandLine& command_line) {
//   return crashpad::CrashHandlerHost::Get()->GetDeathSignalSocket();
// }
// #elif defined(OS_LINUX)
// breakpad::CrashHandlerHostLinux* CreateCrashHandlerHost(
//     const std::string& process_type) {
//   base::FilePath dumps_path =
//       base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(
//           switches::kCrashDumpsDir);
//   {
//     ANNOTATE_SCOPED_MEMORY_LEAK;
//     breakpad::CrashHandlerHostLinux* crash_handler =
//         new breakpad::CrashHandlerHostLinux(
//             process_type, dumps_path, false);
//     crash_handler->StartUploaderThread();
//     return crash_handler;
//   }
// }

// int GetCrashSignalFD(const base::CommandLine& command_line) {
//   if (!breakpad::IsCrashReporterEnabled())
//     return -1;

//   std::string process_type =
//       command_line.GetSwitchValueASCII(switches::kProcessType);

//   if (process_type == switches::kRendererProcess) {
//     static breakpad::CrashHandlerHostLinux* crash_handler = nullptr;
//     if (!crash_handler)
//       crash_handler = CreateCrashHandlerHost(process_type);
//     return crash_handler->GetDeathSignalSocket();
//   }

//   if (process_type == switches::kPpapiPluginProcess) {
//     static breakpad::CrashHandlerHostLinux* crash_handler = nullptr;
//     if (!crash_handler)
//       crash_handler = CreateCrashHandlerHost(process_type);
//     return crash_handler->GetDeathSignalSocket();
//   }

//   if (process_type == switches::kGpuProcess) {
//     static breakpad::CrashHandlerHostLinux* crash_handler = nullptr;
//     if (!crash_handler)
//       crash_handler = CreateCrashHandlerHost(process_type);
//     return crash_handler->GetDeathSignalSocket();
//   }

//   return -1;
// }
// #endif  // defined(OS_ANDROID)

}  // namespace

std::string GetProduct() {
  return "Chromium/" + version_info::GetVersionNumber();
}

std::string GetUserAgent() {
  std::string product = "Bison/1.0 " + GetProduct();
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kUseMobileUserAgent))
    product += " Mobile";
  return content::BuildUserAgentFromProduct(product);
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

BisonContentBrowserClient* BisonContentBrowserClient::Get() {
  return g_browser_client;
}

BisonContentBrowserClient::BisonContentBrowserClient()
    : shell_browser_main_parts_(nullptr) {
  DCHECK(!g_browser_client);
  g_browser_client = this;
}

BisonContentBrowserClient::~BisonContentBrowserClient() {
  g_browser_client = nullptr;
}

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

  // network::mojom::NetworkContextParamsPtr context_params =
  //     network::mojom::NetworkContextParams::New();
  // content::UpdateCorsExemptHeader(context_params.get());
  // context_params->user_agent = GetUserAgent();
  // context_params->accept_language = "en-us,en";


#if DCHECK_IS_ON()
  g_created_network_context_params = true;
#endif
  content::GetNetworkService()->CreateNetworkContext(
      network_context.BindNewPipeAndPassReceiver(), std::move(context_params));
  return network_context;
}

BisonBrowserContext* BisonContentBrowserClient::InitBrowserContext() {
  browser_context_ = std::make_unique<BisonBrowserContext>();
  return browser_context_.get();
}

std::unique_ptr<BrowserMainParts>
BisonContentBrowserClient::CreateBrowserMainParts(
    const MainFunctionParams& parameters) {
  VLOG(0) << "CreateBrowserMainParts";
  auto browser_main_parts = std::make_unique<BisonBrowserMainParts>(this);
  shell_browser_main_parts_ = browser_main_parts.get();
  return browser_main_parts;
}

bool BisonContentBrowserClient::IsHandledURL(const GURL& url) {
  const std::string path = url.path();
  VLOG(0) << "isHandledURL" << path;
  if (!url.is_valid())
    return false;
  // Keep in sync with ProtocolHandlers added by
  // ShellURLRequestContextGetter::GetURLRequestContext().
  static const char* const kProtocolList[] = {
      url::kBlobScheme,         url::kFileSystemScheme,
      content::kChromeUIScheme, content::kChromeDevToolsScheme,
      url::kDataScheme,         url::kFileScheme,
  };
  for (size_t i = 0; i < base::size(kProtocolList); ++i) {
    if (url.scheme() == kProtocolList[i])
      return true;
  }
  return net::URLRequest::IsHandledProtocol(url.scheme());
}

void BisonContentBrowserClient::BindInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  if (!frame_interfaces_) {
    frame_interfaces_ = std::make_unique<
        service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>>();
    ExposeInterfacesToFrame(frame_interfaces_.get());
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
  return "en-us,en";
}

std::string BisonContentBrowserClient::GetDefaultDownloadName() {
  NOTREACHED() << "BisonView does not use chromium downloads";
  return std::string();
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
  return GeneratedCodeCacheSettings(true, 0, context->GetPath());
}

base::OnceClosure BisonContentBrowserClient::SelectClientCertificate(
    WebContents* web_contents,
    net::SSLCertRequestInfo* cert_request_info,
    net::ClientCertIdentityList client_certs,
    std::unique_ptr<ClientCertificateDelegate> delegate) {
  if (select_client_certificate_callback_)
    std::move(select_client_certificate_callback_).Run();
  return base::OnceClosure();
}

// SpeechRecognitionManagerDelegate*
// BisonContentBrowserClient::CreateSpeechRecognitionManagerDelegate() {
//   return new ShellSpeechRecognitionManagerDelegate();
// }

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
    throttles.push_back(page_load_metrics::MetricsNavigationThrottle::Create(
        navigation_handle));
    throttles.push_back(
        navigation_interception::InterceptNavigationDelegate::CreateThrottleFor(
            navigation_handle, navigation_interception::SynchronyMode::kSync));

    // throttles.push_back(std::make_unique<PolicyBlacklistNavigationThrottle>(
    //     navigation_handle, BisonBrowserContext::FromWebContents(
    //                            navigation_handle->GetWebContents())));
  }
  return throttles;
}

DevToolsManagerDelegate*
BisonContentBrowserClient::GetDevToolsManagerDelegate() {
  return new BisonDevToolsManagerDelegate();
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
  if (!login_request_callback_.is_null()) {
    std::move(login_request_callback_).Run(is_main_frame);
  }
  return nullptr;
}

std::string BisonContentBrowserClient::GetProduct() {
  return bison::GetProduct();
}

std::string BisonContentBrowserClient::GetUserAgent() {
  return bison::GetUserAgent();
}

void BisonContentBrowserClient::GetAdditionalMappedFilesForChildProcess(
    const base::CommandLine& command_line,
    int child_process_id,
    content::PosixFileDescriptorInfo* mappings) {
  mappings->ShareWithRegion(
      kBisonPakDescriptor,
      base::GlobalDescriptors::GetInstance()->Get(kBisonPakDescriptor),
      base::GlobalDescriptors::GetInstance()->GetRegion(kBisonPakDescriptor));
  // jiang 这里可以定义v8 的文件位置？

  // int crash_signal_fd = GetCrashSignalFD(command_line);
  // if (crash_signal_fd >= 0) {
  //   mappings->Share(service_manager::kCrashDumpSignal, crash_signal_fd);
  // }
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

// BisonBrowserContext* BisonContentBrowserClient::browser_context() {
//   return shell_browser_main_parts_->browser_context();
// }

void BisonContentBrowserClient::ExposeInterfacesToFrame(
    service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>*
        registry) {}

}  // namespace bison
