
#include "bison_main_delegate.h"

#include <iostream>

#include "bison/browser/bison_content_browser_client.h"
#include "bison/browser/bison_media_url_interceptor.h"
#include "bison/browser/scoped_add_feature_flags.h"
#include "bison/common/bison_resource_bundle.h"
#include "bison/common/bison_content_client.h"
#include "bison/common/bison_descriptors.h"
#include "bison/common/bison_switches.h"
#include "bison/gpu/bison_content_gpu_client.h"
#include "bison/renderer/bison_content_renderer_client.h"

#include "base/android/apk_assets.h"
#include "base/android/build_info.h"
#include "base/bind.h"
#include "base/check_op.h"
#include "base/command_line.h"
#include "base/cpu.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/trace_event/trace_log.h"
#include "build/build_config.h"
#include "cc/base/switches.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/crash/core/common/crash_key.h"
#include "components/gwp_asan/buildflags/buildflags.h"
#include "components/spellcheck/spellcheck_buildflags.h"
#include "components/viz/common/features.h"
#include "content/public/browser/android/compositor.h"
#include "content/public/browser/android/media_url_interceptor_register.h"
#include "content/public/browser/browser_main_runner.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_descriptor_keys.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "gin/public/isolate_holder.h"
#include "gin/v8_initializer.h"
#include "gpu/command_buffer/service/gpu_switches.h"
#include "gpu/config/gpu_finch_features.h"
#include "gpu/ipc/gl_in_process_context.h"
#include "media/base/media_switches.h"
#include "media/media_buildflags.h"
#include "services/network/public/cpp/features.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/resource/resource_bundle_android.h"
#include "ui/base/ui_base_paths.h"
#include "ui/base/ui_base_switches.h"
#include "ui/events/gesture_detection/gesture_configuration.h"

#if BUILDFLAG(ENABLE_SPELLCHECK)
#include "components/spellcheck/common/spellcheck_features.h"
#endif  // ENABLE_SPELLCHECK

#if BUILDFLAG(ENABLE_GWP_ASAN)
#include "components/gwp_asan/client/gwp_asan.h"  // nogncheck
#endif

namespace {

void InitLogging(const base::CommandLine* command_line) {
  base::FilePath log_filename =
      command_line->GetSwitchValuePath(switches::kLogFile);
  if (log_filename.empty()) {
    base::PathService::Get(base::DIR_EXE, &log_filename);
    log_filename = log_filename.AppendASCII("bison.log");
  }

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_ALL;
  settings.log_file_path = log_filename.value().c_str();
  settings.delete_old = logging::DELETE_OLD_LOG_FILE;
  logging::InitLogging(settings);
  logging::SetLogItems(true /* Process ID */, true /* Thread ID */,
                       true /* Timestamp */, false /* Tick count */);
  VLOG(0) << "log file at:" << log_filename.value().c_str();
}

}  // namespace

namespace bison {

BisonMainDelegate::BisonMainDelegate() {}

BisonMainDelegate::~BisonMainDelegate() {}

bool BisonMainDelegate::BasicStartupComplete(int* exit_code) {
  SetContentClient(&content_client_);

  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();
  InitLogging(cl);

  cl->AppendSwitch(switches::kDisableOverscrollEdgeEffect);

  cl->AppendSwitch(switches::kDisablePullToRefreshEffect);

  // Not yet supported in single-process mode.
  cl->AppendSwitch(switches::kDisableSharedWorkers);

  cl->AppendSwitch(switches::kDisableFileSystem);

  // Web Notification API and the Push API are not supported (crbug.com/434712)
  cl->AppendSwitch(switches::kDisableNotifications);

  cl->AppendSwitch(switches::kDisableSpeechSynthesisAPI);

  cl->AppendSwitch(switches::kDisablePermissionsAPI);

  cl->AppendSwitch(switches::kEnableAggressiveDOMStorageFlushing);

  cl->AppendSwitch(switches::kDisablePresentationAPI);

  cl->AppendSwitch(switches::kDisableRemotePlaybackAPI);

  cl->AppendSwitch(switches::kDisableMediaSessionAPI);

#if defined(V8_USE_EXTERNAL_STARTUP_DATA)
  if (cl->GetSwitchValueASCII(switches::kProcessType).empty()) {
    // Browser process (no type specified).

    content::RegisterMediaUrlInterceptor(new BisonMediaUrlInterceptor());
    // BrowserViewRenderer::CalculateTileMemoryPolicy();

    // WebView apps can override WebView#computeScroll to achieve custom
    // scroll/fling. As a result, fling animations may not be ticked,
    // potentially
    // confusing the tap suppression controller. Simply disable it for WebView
    ui::GestureConfiguration::GetInstance()
        ->set_fling_touchscreen_tap_suppression_enabled(false);

#if defined(USE_V8_CONTEXT_SNAPSHOT)
    VLOG(0) << "define USE_V8_CONTEXT_SNAPSHOT";
#if defined(ARCH_CPU_ARM_FAMILY)
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot32DataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/arm/v8_context_snapshot_32.bin"));
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot64DataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/arm/v8_context_snapshot_64.bin"));
#else
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot32DataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/x86/v8_context_snapshot_32.bin"));
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot64DataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/x86/v8_context_snapshot_64.bin"));
#endif  // ARCH_CPU_ARM_FAMILY
#else
    VLOG(0) << "undefine USE_V8_CONTEXT_SNAPSHOT";
#if defined(ARCH_CPU_ARM_FAMILY)
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot32DataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/arm/snapshot_blob_32.bin"));
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot64DataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/arm/snapshot_blob_64.bin"));
#else
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot32DataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/x86/snapshot_blob_32.bin"));
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot64DataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/x86/snapshot_blob_64.bin"));
#endif  // ARCH_CPU_ARM_FAMILY
#endif
  }
#endif  // V8_USE_EXTERNAL_STARTUP_DATA

  // jiang 后面有世界加
  // if (cl->HasSwitch(switches::kWebViewSandboxedRenderer)) {
  //   content::RenderProcessHost::SetMaxRendererProcessCount(1u);
  //   cl->AppendSwitch(switches::kInProcessGPU);
  // }

  {
    ScopedAddFeatureFlags features(cl);

    features.EnableIfNotSet(
        autofill::features::kAutofillSkipComparingInferredLabels);

    // 外面加开关？
    features.EnableIfNotSet(::features::kLogJsConsoleMessages);

    features.DisableIfNotSet(::features::kWebPayments);

    features.DisableIfNotSet(::features::kWebAuth);

    // FATAL:compositor_impl_android.cc(299)] Check failed:
    // features::IsVizDisplayCompositorEnabled().
    // features.DisableIfNotSet(::features::kVizDisplayCompositor);
    features.EnableIfNotSet(media::kDisableSurfaceLayerForVideo);

    features.DisableIfNotSet(media::kMediaDrmPersistentLicense);

    features.DisableIfNotSet(media::kPictureInPictureAPI);

    features.DisableIfNotSet(::features::kBackgroundFetch);

    features.EnableIfNotSet(::features::kDisableSurfaceControlForWebview);
    features.DisableIfNotSet(::features::kSmsReceiver);

    features.DisableIfNotSet(::features::kWebXr);

    features.DisableIfNotSet(::features::kWebXrArModule);

    features.DisableIfNotSet(::features::kWebXrHitTest);

    // features.EnableIfNotSet(::features::kDisableDeJelly);

    features.DisableIfNotSet(network::features::kCrossOriginEmbedderPolicy);
    features.DisableIfNotSet(::features::kInstalledApp);
    // features.EnableIfNotSet(
    //     metrics::UnsentLogStoreMetrics::kRecordLastUnsentLogMetadataMetrics);
  }

  content::Compositor::Initialize();

  return false;
}

void BisonMainDelegate::PreSandboxStartup() {
  TRACE_EVENT0("startup", "BisonMainDelegate::PreSandboxStartup");
#if defined(ARCH_CPU_ARM_FAMILY)
  // Create an instance of the CPU class to parse /proc/cpuinfo and cache
  // cpu_brand info.
  base::CPU cpu_info;
#endif

  crash_reporter::InitializeCrashKeys();
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  std::string process_type =
      command_line.GetSwitchValueASCII(switches::kProcessType);
  const bool is_browser_process = process_type.empty();
  if (!is_browser_process) {
    base::i18n::SetICUDefaultLocale(
        command_line.GetSwitchValueASCII(switches::kLang));
  }

  if (process_type == switches::kRendererProcess) {
    InitResourceBundleRendererSide();
  }
}

int BisonMainDelegate::RunProcess(
    const std::string& process_type,
    const MainFunctionParams& main_function_params) {
  // For non-browser process, return and have the caller run the main loop.
  if (!process_type.empty())
    return -1;

  base::trace_event::TraceLog::GetInstance()->set_process_name("Browser");

  // On Android, we defer to the system message loop when the stack unwinds.
  // So here we only create (and leak) a BrowserMainRunner. The shutdown
  // of BrowserMainRunner doesn't happen in Chrome Android and doesn't work
  // properly on Android at all.
  std::unique_ptr<content::BrowserMainRunner> main_runner =
      content::BrowserMainRunner::Create();
  // In browser tests, the |main_function_params| contains a |ui_task| which
  // will execute the testing. The task will be executed synchronously inside
  // Initialize() so we don't depend on the BrowserMainRunner being Run().
  int initialize_exit_code = main_runner->Initialize(main_function_params);
  DCHECK_LT(initialize_exit_code, 0)
      << "BrowserMainRunner::Initialize failed in BisonMainDelegate";
  ignore_result(main_runner.release());
  // Return 0 as BrowserMain() should not be called after this, bounce up to
  // the system message loop for ContentShell, and we're already done thanks
  // to the |ui_task| for browser tests.
  return 0;
}

void BisonMainDelegate::ProcessExiting(const std::string& process_type) {
  logging::CloseLogFile();
}

bool BisonMainDelegate::ShouldCreateFeatureList() {
  return true;
}

// This function is called only on the browser process.
void BisonMainDelegate::PostEarlyInitialization(bool is_running_tests) {
  InitIcuAndResourceBundleBrowserSide();
  bison_feature_list_creator_->CreateFeatureListAndFieldTrials();
  PostFieldTrialInitialization();
}

void BisonMainDelegate::PostFieldTrialInitialization() {

}



ContentBrowserClient* BisonMainDelegate::CreateContentBrowserClient() {
  bison_feature_list_creator_ = std::make_unique<BisonFeatureListCreator>();
  browser_client_.reset(
      new BisonContentBrowserClient(bison_feature_list_creator_.get()));
  return browser_client_.get();
}

ContentGpuClient* BisonMainDelegate::CreateContentGpuClient() {
  gpu_client_ = std::make_unique<BisonContentGpuClient>();
  return gpu_client_.get();
}

ContentRendererClient* BisonMainDelegate::CreateContentRendererClient() {
  renderer_client_.reset(new BisonContentRendererClient);
  return renderer_client_.get();
}

}  // namespace bison
