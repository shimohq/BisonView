
#include "bison_main_delegate.h"

#include <iostream>

#include "bison/browser/bison_media_url_interceptor.h"

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/cpu.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/trace_event/trace_log.h"
#include "bison/browser/bison_content_browser_client.h"
#include "bison/browser/scoped_add_feature_flags.h"
#include "bison/common/bison_content_client.h"
#include "bison/common/bison_descriptors.h"
#include "bison/gpu/bison_content_gpu_client.h"
#include "bison/renderer/bison_content_renderer_client.h"
#include "build/build_config.h"
#include "cc/base/switches.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/crash/core/common/crash_key.h"
#include "components/viz/common/features.h"
#include "components/viz/common/switches.h"
#include "content/common/content_constants_internal.h"
#include "content/public/browser/android/media_url_interceptor_register.h"
#include "content/public/browser/browser_main_runner.h"
#include "content/public/common/content_descriptor_keys.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "gin/v8_initializer.h"
#include "gpu/config/gpu_finch_features.h"
#include "gpu/config/gpu_switches.h"
#include "ipc/ipc_buildflags.h"
#include "media/base/media_switches.h"
#include "net/cookies/cookie_monster.h"
#include "ppapi/buildflags/buildflags.h"
#include "services/network/public/cpp/network_switches.h"
#include "services/service_manager/embedder/switches.h"
#include "skia/ext/test_fonts.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"
#include "ui/base/ui_base_switches.h"
#include "ui/display/display_switches.h"
#include "ui/events/gesture_detection/gesture_configuration.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_switches.h"

#if BUILDFLAG(IPC_MESSAGE_LOG_ENABLED)
#define IPC_MESSAGE_MACROS_LOG_ENABLED
#include "content/public/common/content_ipc_logging.h"
#define IPC_LOG_TABLE_ADD_ENTRY(msg_id, logger) \
  content::RegisterIPCLogger(msg_id, logger)
#endif

#include "base/android/apk_assets.h"
#include "base/posix/global_descriptors.h"
#include "components/crash/content/app/crashpad.h"  // nogncheck
#include "content/public/browser/android/compositor.h"
#include "content/public/test/nested_message_pump_android.h"

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

  cl->AppendSwitch(switches::kDisableSharedWorkers);

  cl->AppendSwitch(switches::kDisableFileSystem);

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
    VLOG(0) << "defined USE_V8_CONTEXT_SNAPSHOT";
    ui::GestureConfiguration::GetInstance()
        ->set_fling_touchscreen_tap_suppression_enabled(false);

    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8NativesDataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/natives_blob.bin"));
// #if defined(USE_V8_CONTEXT_SNAPSHOT)
//     VLOG(0) << "defined USE_V8_CONTEXT_SNAPSHOT";
//     gin::V8Initializer::V8SnapshotFileType file_type =
//         gin::V8Initializer::V8SnapshotFileType::kWithAdditionalContext;
// #else
//     gin::V8Initializer::V8SnapshotFileType file_type =
//         gin::V8Initializer::V8SnapshotFileType::kDefault;
// #endif
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
  }
#endif  // V8_USE_EXTERNAL_STARTUP_DATA

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

    //FATAL:compositor_impl_android.cc(299)] Check failed: features::IsVizDisplayCompositorEnabled(). 
    // features.DisableIfNotSet(::features::kVizDisplayCompositor);

    features.DisableIfNotSet(media::kUseAndroidOverlay);

    features.EnableIfNotSet(media::kDisableSurfaceLayerForVideo);

    features.DisableIfNotSet(media::kMediaDrmPersistentLicense);

    features.DisableIfNotSet(media::kPictureInPictureAPI);

    // features.DisableIfNotSet(
    //     autofill::features::kAutofillRestrictUnownedFieldsToFormlessCheckout);

    features.DisableIfNotSet(::features::kBackgroundFetch);

    features.DisableIfNotSet(::features::kAndroidSurfaceControl);

    features.DisableIfNotSet(::features::kSmsReceiver);

    features.DisableIfNotSet(::features::kWebXr);

    // features.EnableIfNotSet(::features::kDisableDeJelly);
  }

  content::Compositor::Initialize();

  return false;
}

void BisonMainDelegate::PreSandboxStartup() {
#if defined(ARCH_CPU_ARM_FAMILY)
  // Create an instance of the CPU class to parse /proc/cpuinfo and cache
  // cpu_brand info.
  base::CPU cpu_info;
#endif

  VLOG(0) << "======调试宏定义======";
#if defined(ARCH_CPU_ARM_FAMILY)
  VLOG(0) << "defined(ARCH_CPU_ARM_FAMILY) true";
#else
  VLOG(0) << "defined(ARCH_CPU_ARM_FAMILY) false";
#endif

#if defined(OS_FUCHSIA)
  VLOG(0) << "defined(OS_FUCHSIA) true";
#else
  VLOG(0) << "defined(OS_FUCHSIA) false";
#endif

#if defined(TOOLKIT_VIEWS)
  VLOG(0) << "defined(TOOLKIT_VIEWS) true";
#else
  VLOG(0) << "defined(TOOLKIT_VIEWS) false";
#endif

#if BUILDFLAG(IPC_MESSAGE_LOG_ENABLED)
  VLOG(0) << "BUILDFLAG(IPC_MESSAGE_LOG_ENABLED) true";
#else
  VLOG(0) << "BUILDFLAG(IPC_MESSAGE_LOG_ENABLED) false";
#endif
  VLOG(0) << "======调试宏定义======";

  crash_reporter::InitializeCrashKeys();

  InitializeResourceBundle();
}

int BisonMainDelegate::RunProcess(
    const std::string& process_type,
    const MainFunctionParams& main_function_params) {
  // For non-browser process, return and have the caller run the main loop.
  if (!process_type.empty())
    return -1;

  base::trace_event::TraceLog::GetInstance()->set_process_name("Browser");
  base::trace_event::TraceLog::GetInstance()->SetProcessSortIndex(
      content::kTraceEventBrowserProcessSortIndex);

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
  return false;
}

// This function is called only on the browser process.
void BisonMainDelegate::PostEarlyInitialization(bool is_running_tests) {
  // InitIcuAndResourceBundleBrowserSide();
  bison_feature_list_creator_->CreateFeatureListAndFieldTrials();
  PostFieldTrialInitialization();
}

void BisonMainDelegate::InitializeResourceBundle() {
  // On Android, the renderer runs with a different UID and can never access
  // the file system. Use the file descriptor passed in at launch time.
  auto* global_descriptors = base::GlobalDescriptors::GetInstance();
  int pak_fd = global_descriptors->MaybeGet(kBisonPakDescriptor);
  base::MemoryMappedFile::Region pak_region;
  if (pak_fd >= 0) {
    pak_region = global_descriptors->GetRegion(kBisonPakDescriptor);
  } else {
    pak_fd = base::android::OpenApkAsset("assets/bison.pak", &pak_region);
    // Loaded from disk for browsertests.
    if (pak_fd < 0) {
      base::FilePath pak_file;
      bool r = base::PathService::Get(base::DIR_ANDROID_APP_DATA, &pak_file);
      DCHECK(r);
      pak_file = pak_file.Append(FILE_PATH_LITERAL("paks"));
      pak_file = pak_file.Append(FILE_PATH_LITERAL("bison.pak"));
      int flags = base::File::FLAG_OPEN | base::File::FLAG_READ;
      pak_fd = base::File(pak_file, flags).TakePlatformFile();
      pak_region = base::MemoryMappedFile::Region::kWholeFile;
    }
    global_descriptors->Set(kBisonPakDescriptor, pak_fd, pak_region);
  }
  DCHECK_GE(pak_fd, 0);
  // This is clearly wrong. See crbug.com/330930
  ui::ResourceBundle::InitSharedInstanceWithPakFileRegion(base::File(pak_fd),
                                                          pak_region);
  ui::ResourceBundle::GetSharedInstance().AddDataPackFromFileRegion(
      base::File(pak_fd), pak_region, ui::SCALE_FACTOR_100P);
}

void BisonMainDelegate::PreCreateMainMessageLoop() {}

ContentBrowserClient* BisonMainDelegate::CreateContentBrowserClient() {
  bison_feature_list_creator_ = std::make_unique<BisonFeatureListCreator>();
  browser_client_.reset(
      new BisonContentBrowserClient(bison_feature_list_creator_.get()));
  return browser_client_.get();
}

ContentGpuClient* BisonMainDelegate::CreateContentGpuClient() {
  // gpu_client_.reset(new BisonContentGpuClient);
  gpu_client_ = std::make_unique<BisonContentGpuClient>();
  return gpu_client_.get();
}

ContentRendererClient* BisonMainDelegate::CreateContentRendererClient() {
  renderer_client_.reset(new BisonContentRendererClient);
  return renderer_client_.get();
}

}  // namespace bison
