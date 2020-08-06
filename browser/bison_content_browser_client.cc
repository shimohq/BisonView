#include "bison_content_browser_client.h"

#include "bison_browser_context.h"
#include "bison_browser_main_parts.h"
#include "bison_devtools_manager_delegate.h"
// #include "content/shell/browser/shell.h"

// #include "content/shell/browser/shell_quota_permission_context.h"
// #include "content/shell/browser/shell_web_contents_view_delegate_creator.h"
// #include "content/shell/common/power_monitor_test.mojom.h"
// #include "content/shell/common/shell_switches.h"

#include <stddef.h>
#include <utility>

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "content/public/browser/cors_exempt_headers.h"
#include "content/public/browser/login_delegate.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/user_agent.h"
#include "content/public/common/web_preferences.h"
#include "content/public/test/test_service.h"

#include "content/test/data/mojo_web_test_helper_test.mojom.h"
#include "device/bluetooth/public/mojom/test/fake_bluetooth.mojom.h"
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

#if defined(OS_ANDROID)
#include "base/android/apk_assets.h"
#include "base/android/path_utils.h"
#include "components/crash/content/app/crashpad.h"
#include "content/shell/android/shell_descriptors.h"
#endif

#if defined(OS_CHROMEOS)
#include "content/public/browser/context_factory.h"
#endif

#if defined(OS_LINUX) || defined(OS_ANDROID)
#include "base/debug/leak_annotations.h"
#include "components/crash/content/browser/crash_handler_host_linux.h"
#include "content/public/common/content_descriptors.h"
#endif

#if defined(OS_WIN)
#include "sandbox/win/src/sandbox.h"
#include "services/service_manager/sandbox/win/sandbox_win.h"
#endif

#if BUILDFLAG(ENABLE_MOJO_MEDIA_IN_BROWSER_PROCESS) || \
    BUILDFLAG(ENABLE_CAST_RENDERER)
#include "media/mojo/mojom/constants.mojom.h"           // nogncheck
#include "media/mojo/services/media_service_factory.h"  // nogncheck
#endif

using content::StoragePartition;

namespace bison {

namespace {

BisonContentBrowserClient* g_browser_client;

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

// const service_manager::Manifest& GetContentBrowserOverlayManifest() {
//   static base::NoDestructor<service_manager::Manifest> manifest{
//       service_manager::ManifestBuilder()
//           .ExposeCapability(
//               "renderer",
//               service_manager::Manifest::InterfaceList<
//                   mojom::MojoWebTestHelper, mojom::FakeBluetoothChooser,
//                   mojom::FakeBluetoothChooserFactory,
//                   mojom::WebTestBluetoothFakeAdapterSetter,
//                   bluetooth::mojom::FakeBluetooth>())
//           .ExposeInterfaceFilterCapability_Deprecated(
//               "navigation:frame", "renderer",
//               service_manager::Manifest::InterfaceList<
//                   mojom::MojoWebTestHelper>())
//           .Build()};
//   return *manifest;
// }

// const service_manager::Manifest& GetContentGpuOverlayManifest() {
//   static base::NoDestructor<service_manager::Manifest> manifest{
//       service_manager::ManifestBuilder()
//           .ExposeCapability("browser",
//           service_manager::Manifest::InterfaceList<
//                                            mojom::PowerMonitorTest>())
//           .Build()};
//   return *manifest;
// }

// const service_manager::Manifest& GetContentRendererOverlayManifest() {
//   static base::NoDestructor<service_manager::Manifest> manifest{
//       service_manager::ManifestBuilder()
//           .ExposeCapability(
//               "browser",
//               service_manager::Manifest::InterfaceList<mojom::PowerMonitorTest,
//                                                        mojom::TestService>())
//           .ExposeInterfaceFilterCapability_Deprecated(
//               "navigation:frame", "browser",
//               service_manager::Manifest::InterfaceList<mojom::WebTestControl>())
//           .Build()};
//   return *manifest;
// }

// const service_manager::Manifest& GetContentUtilityOverlayManifest() {
//   static base::NoDestructor<service_manager::Manifest> manifest{
//       service_manager::ManifestBuilder()
//           .ExposeCapability(
//               "browser",
//               service_manager::Manifest::InterfaceList<mojom::PowerMonitorTest,
//                                                        mojom::TestService>())
//           .Build()};
//   return *manifest;
// }

}  // namespace

std::string GetBisonUserAgent() {
  std::string product = "Chrome/bison_1";
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (base::FeatureList::IsEnabled(blink::features::kFreezeUserAgent)) {
    return content::GetFrozenUserAgent(
               command_line->HasSwitch(switches::kUseMobileUserAgent))
        .as_string();
  }
  if (command_line->HasSwitch(switches::kUseMobileUserAgent))
    product += " Mobile";
  return content::BuildUserAgentFromProduct(product);
}

blink::UserAgentMetadata GetBisonUserAgentMetadata() {
  blink::UserAgentMetadata metadata;

  metadata.brand = "bison";
  // metadata.full_version = CONTENT_VERSION;
  metadata.full_version = "99.77.34.5";
  metadata.major_version = "99";
  metadata.platform = content::BuildOSCpuInfo(false);

  // TODO(mkwst): Split these out from BuildOSCpuInfo().
  metadata.architecture = "";
  metadata.model = "";

  return metadata;
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

std::unique_ptr<BrowserMainParts>
BisonContentBrowserClient::CreateBrowserMainParts(
    const MainFunctionParams& parameters) {
  VLOG(0) << "CreateBrowserMainParts";
  auto browser_main_parts = std::make_unique<BisonBrowserMainParts>(parameters);

  shell_browser_main_parts_ = browser_main_parts.get();

  return browser_main_parts;
}

bool BisonContentBrowserClient::IsHandledURL(const GURL& url) {
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

  if (is_media_service) {
    service_manager::Service::RunAsyncUntilTermination(
        media::CreateMediaServiceForTesting(std::move(*receiver)));
  }
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
  VLOG(0) << "GetServiceManifestOverlay : name " << name;
  // if (name == content::mojom::kBrowserServiceName)
  //   return GetContentBrowserOverlayManifest();
  // if (name == content::mojom::kGpuServiceName)
  // return GetContentGpuOverlayManifest();
  // if (name == content::mojom::kRendererServiceName)
  // return GetContentRendererOverlayManifest();
  // if (name == content::mojom::kUtilityServiceName)
  // return GetContentUtilityOverlayManifest();

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
  return "download";
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
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kForceDarkMode)) {
    prefs->preferred_color_scheme = blink::PreferredColorScheme::kDark;
  } else {
    prefs->preferred_color_scheme = blink::PreferredColorScheme::kLight;
  }
}

base::FilePath BisonContentBrowserClient::GetFontLookupTableCacheDir() {
  return browser_context()->GetPath().Append(
      FILE_PATH_LITERAL("FontLookupTableCache"));
}

DevToolsManagerDelegate*
BisonContentBrowserClient::GetDevToolsManagerDelegate() {
  return new BisonDevToolsManagerDelegate(browser_context());
}

void BisonContentBrowserClient::OpenURL(
    SiteInstance* site_instance,
    const OpenURLParams& params,
    base::OnceCallback<void(WebContents*)> callback) {
  // std::move(callback).Run(
  //     Shell::CreateNewWindow(site_instance->GetBrowserContext(), params.url,
  //                            nullptr, gfx::Size())
  //         ->web_contents());
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

std::string BisonContentBrowserClient::GetUserAgent() {
  return GetBisonUserAgent();
}

blink::UserAgentMetadata BisonContentBrowserClient::GetUserAgentMetadata() {
  return GetBisonUserAgentMetadata();
}

// #if defined(OS_LINUX) || defined(OS_ANDROID)
// void BisonContentBrowserClient::GetAdditionalMappedFilesForChildProcess(
//     const base::CommandLine& command_line,
//     int child_process_id,
//     content::PosixFileDescriptorInfo* mappings) {
// #if defined(OS_ANDROID)
//   mappings->ShareWithRegion(
//       kShellPakDescriptor,
//       base::GlobalDescriptors::GetInstance()->Get(kShellPakDescriptor),
//       base::GlobalDescriptors::GetInstance()->GetRegion(kShellPakDescriptor));
// #endif
//   int crash_signal_fd = GetCrashSignalFD(command_line);
//   if (crash_signal_fd >= 0) {
//     mappings->Share(service_manager::kCrashDumpSignal, crash_signal_fd);
//   }
// }
// #endif  // defined(OS_LINUX) || defined(OS_ANDROID)

// #if defined(OS_WIN)
// bool BisonContentBrowserClient::PreSpawnRenderer(sandbox::TargetPolicy*
// policy,
//                                                  RendererSpawnFlags flags) {
//   // Add sideloaded font files for testing. See also DIR_WINDOWS_FONTS
//   // addition in |StartSandboxedProcess|.
//   std::vector<std::string> font_files = switches::GetSideloadFontFiles();
//   for (std::vector<std::string>::const_iterator i(font_files.begin());
//        i != font_files.end(); ++i) {
//     policy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES,
//                     sandbox::TargetPolicy::FILES_ALLOW_READONLY,
//                     base::UTF8ToWide(*i).c_str());
//   }
//   return true;
// }
// #endif  // OS_WIN

mojo::Remote<network::mojom::NetworkContext>
BisonContentBrowserClient::CreateNetworkContext(
    BrowserContext* context,
    bool in_memory,
    const base::FilePath& relative_partition_path) {
  mojo::Remote<network::mojom::NetworkContext> network_context;
  network::mojom::NetworkContextParamsPtr context_params =
      network::mojom::NetworkContextParams::New();
  content::UpdateCorsExemptHeader(context_params.get());
  context_params->user_agent = GetUserAgent();
  context_params->accept_language = "en-us,en";

#if BUILDFLAG(ENABLE_REPORTING)
  // if (base::CommandLine::ForCurrentProcess()->HasSwitch(
  //         switches::kRunWebTests)) {
  //   // Configure the Reporting service in a manner expected by certain Web
  //   // Platform Tests (network-error-logging and reporting-api).
  //   //
  //   //   (1) Always send reports (irrespective of BACKGROUND_SYNC permission)
  //   //   (2) Lower the timeout for sending reports.
  //   context_params->reporting_delivery_interval =
  //       kReportingDeliveryIntervalTimeForWebTests;
  //   context_params->skip_reporting_send_permission_check = true;
  // }
#endif

  content::GetNetworkService()->CreateNetworkContext(
      network_context.BindNewPipeAndPassReceiver(), std::move(context_params));
  return network_context;
}

std::vector<base::FilePath>
BisonContentBrowserClient::GetNetworkContextsParentDirectory() {
  return {browser_context()->GetPath()};
}

BisonBrowserContext* BisonContentBrowserClient::browser_context() {
  return shell_browser_main_parts_->browser_context();
}

BisonBrowserContext*
BisonContentBrowserClient::off_the_record_browser_context() {
  return shell_browser_main_parts_->off_the_record_browser_context();
}

void BisonContentBrowserClient::ExposeInterfacesToFrame(
    service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>*
        registry) {}

}  // namespace bison