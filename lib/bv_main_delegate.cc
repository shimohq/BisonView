
#include "bv_main_delegate.h"

#include <iostream>

#include "bison/browser/bv_content_browser_client.h"
#include "bison/browser/bv_media_url_interceptor.h"
#include "bison/browser/scoped_add_feature_flags.h"
#include "bison/common/bv_resource_bundle.h"
#include "bison/common/bv_content_client.h"
#include "bison/common/bv_descriptors.h"
#include "bison/common/bv_switches.h"
#include "bison/gpu/bv_content_gpu_client.h"
#include "bison/renderer/bv_content_renderer_client.h"

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

namespace bison {

BvMainDelegate::BvMainDelegate() {}

BvMainDelegate::~BvMainDelegate() {}

bool BvMainDelegate::BasicStartupComplete(int* exit_code) {
  SetContentClient(&content_client_);

  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();

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

    content::RegisterMediaUrlInterceptor(new BvMediaUrlInterceptor());
    // BrowserViewRenderer::CalculateTileMemoryPolicy();

    // WebView apps can override WebView#computeScroll to achieve custom
    // scroll/fling. As a result, fling animations may not be ticked,
    // potentially
    // confusing the tap suppression controller. Simply disable it for WebView
    ui::GestureConfiguration::GetInstance()
        ->set_fling_touchscreen_tap_suppression_enabled(false);

#if defined(USE_V8_CONTEXT_SNAPSHOT)
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

void BvMainDelegate::PreSandboxStartup() {
  TRACE_EVENT0("startup", "BvMainDelegate::PreSandboxStartup");
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

int BvMainDelegate::RunProcess(
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
      << "BrowserMainRunner::Initialize failed in BvMainDelegate";
  ignore_result(main_runner.release());
  // Return 0 as BrowserMain() should not be called after this, bounce up to
  // the system message loop for ContentShell, and we're already done thanks
  // to the |ui_task| for browser tests.
  return 0;
}

void BvMainDelegate::ProcessExiting(const std::string& process_type) {
  logging::CloseLogFile();
}

bool BvMainDelegate::ShouldCreateFeatureList() {
  return true;
}

// This function is called only on the browser process.
void BvMainDelegate::PostEarlyInitialization(bool is_running_tests) {
  InitIcuAndResourceBundleBrowserSide();
  bison_feature_list_creator_->CreateFeatureListAndFieldTrials();
  PostFieldTrialInitialization();
}

void BvMainDelegate::PostFieldTrialInitialization() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line.GetSwitchValueASCII(switches::kProcessType);
  bool is_browser_process = process_type.empty();

  ALLOW_UNUSED_LOCAL(is_browser_process);

#if BUILDFLAG(ENABLE_GWP_ASAN_MALLOC)
  gwp_asan::EnableForMalloc(is_browser_process,
                            process_type.c_str());
#endif

#if BUILDFLAG(ENABLE_GWP_ASAN_PARTITIONALLOC)
  gwp_asan::EnableForPartitionAlloc(false, process_type.c_str());
#endif
}



ContentBrowserClient* BvMainDelegate::CreateContentBrowserClient() {
  bison_feature_list_creator_ = std::make_unique<BvFeatureListCreator>();
  browser_client_.reset(
      new BvContentBrowserClient(bison_feature_list_creator_.get()));
  return browser_client_.get();
}

ContentGpuClient* BvMainDelegate::CreateContentGpuClient() {
  gpu_client_ = std::make_unique<BvContentGpuClient>();
  return gpu_client_.get();
}

ContentRendererClient* BvMainDelegate::CreateContentRendererClient() {
  renderer_client_.reset(new BvContentRendererClient);
  return renderer_client_.get();
}

}  // namespace bison
