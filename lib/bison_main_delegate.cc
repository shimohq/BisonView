
#include "bison_main_delegate.h"

#include "bison/browser/bison_content_browser_client.h"
#include "bison/common/bison_content_client.h"
#include "bison/gpu/bison_content_gpu_client.h"
#include "bison/renderer/bison_content_renderer_client.h"

// #include "content/shell/app/shell_crash_reporter_client.h"

// #include "content/shell/common/shell_switches.h"

#include <iostream>

#include "base/base_switches.h"
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
#include "components/crash/core/common/crash_key.h"
#include "components/viz/common/switches.h"
#include "content/common/content_constants_internal.h"
#include "content/public/browser/browser_main_runner.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"

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
#include "bison_descriptors.h"
#include "content/public/browser/android/compositor.h"
#include "content/public/test/nested_message_pump_android.h"

#include "components/crash/content/app/crashpad.h"  // nogncheck

namespace {

void InitLogging(const base::CommandLine& command_line) {
  base::FilePath log_filename =
      command_line.GetSwitchValuePath(switches::kLogFile);
  if (log_filename.empty()) {
#if defined(OS_FUCHSIA)
    base::PathService::Get(base::DIR_TEMP, &log_filename);
#else
    base::PathService::Get(base::DIR_EXE, &log_filename);
#endif
    log_filename = log_filename.AppendASCII("content_shell.log");
  }

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_ALL;
  settings.log_file_path = log_filename.value().c_str();
  settings.delete_old = logging::DELETE_OLD_LOG_FILE;
  logging::InitLogging(settings);
  logging::SetLogItems(true /* Process ID */, true /* Thread ID */,
                       true /* Timestamp */, false /* Tick count */);
}

}  // namespace

namespace bison {

BisonMainDelegate::BisonMainDelegate() {}

BisonMainDelegate::~BisonMainDelegate() {}

bool BisonMainDelegate::BasicStartupComplete(int* exit_code) {
  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  int dummy;
  if (!exit_code)
    exit_code = &dummy;

  content::Compositor::Initialize();

  InitLogging(command_line);
  content_client_.reset(new BisonContentClient);
  SetContentClient(content_client_.get());

  return false;
}

void BisonMainDelegate::PreSandboxStartup() {
#if defined(ARCH_CPU_ARM_FAMILY)
  // Create an instance of the CPU class to parse /proc/cpuinfo and cache
  // cpu_brand info.
  base::CPU cpu_info;
#endif

  // Disable platform crash handling and initialize the crash reporter, if
  // requested.
  // TODO(crbug.com/753619): Implement crash reporter integration for Fuchsia.
  // #if !defined(OS_FUCHSIA)
  //   if (base::CommandLine::ForCurrentProcess()->HasSwitch(
  //           switches::kEnableCrashReporter)) {
  //     std::string process_type =
  //         base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
  //             switches::kProcessType);
  //     crash_reporter::SetCrashReporterClient(g_shell_crash_client.Pointer());
  //     crash_reporter::InitializeCrashpad(process_type.empty(), process_type);

  //     VLOG(0) << "OS_FUCHSIA";
  //   }
  // #endif  // !defined(OS_FUCHSIA)

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

void BisonMainDelegate::InitializeResourceBundle() {
  // On Android, the renderer runs with a different UID and can never access
  // the file system. Use the file descriptor passed in at launch time.
  auto* global_descriptors = base::GlobalDescriptors::GetInstance();
  int pak_fd = global_descriptors->MaybeGet(kBisonPakDescriptor);
  base::MemoryMappedFile::Region pak_region;
  if (pak_fd >= 0) {
    pak_region = global_descriptors->GetRegion(kBisonPakDescriptor);
  } else {
    // content_shell.pak
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
  browser_client_.reset(new BisonContentBrowserClient);
  return browser_client_.get();
}

ContentGpuClient* BisonMainDelegate::CreateContentGpuClient() {
  gpu_client_.reset(new BisonContentGpuClient);
  return gpu_client_.get();
}

ContentRendererClient* BisonMainDelegate::CreateContentRendererClient() {
  renderer_client_.reset(new BisonContentRendererClient);
  return renderer_client_.get();
}

}  // namespace bison
