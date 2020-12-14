
#include "bison/browser/bison_browser_main_parts.h"

#include <memory>
#include <set>
#include <string>
#include <utility>

#include "bison/browser/bison_browser_context.h"
#include "bison/browser/bison_browser_terminator.h"
#include "bison/browser/bison_content_browser_client.h"
#include "bison/browser/bison_contents.h"
#include "bison/browser/bison_devtools_manager_delegate.h"
#include "bison/browser/metrics/memory_metrics_logger.h"
#include "bison/browser/network_service/bison_network_change_notifier_factory.h"
#include "bison/common/bison_switches.h"

#include "base/android/apk_assets.h"
#include "base/android/build_info.h"
#include "base/android/memory_pressure_listener_android.h"
#include "base/base_paths_android.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/i18n/rtl.h"
#include "base/message_loop/message_loop_current.h"
#include "base/message_loop/message_pump_type.h"
#include "base/path_service.h"
#include "components/crash/content/browser/child_exit_observer_android.h"
#include "components/embedder_support/android/metrics/memory_metrics_logger.h"
#include "components/heap_profiling/multi_process/supervisor.h"
#include "components/services/heap_profiling/public/cpp/settings.h"
#include "content/public/browser/android/synchronous_compositor.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/result_codes.h"
#include "content/public/common/service_names.mojom.h"
#include "net/android/network_change_notifier_factory_android.h"
#include "net/base/network_change_notifier.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/layout.h"
#include "ui/gl/gl_surface.h"

namespace bison {

BisonBrowserMainParts::BisonBrowserMainParts(
    BisonContentBrowserClient* browser_client)
    : browser_client_(browser_client) {}

BisonBrowserMainParts::~BisonBrowserMainParts() {}

int BisonBrowserMainParts::PreEarlyInitialization() {
  if (!net::NetworkChangeNotifier::GetFactory()) {
    net::NetworkChangeNotifier::SetFactory(
        new BisonNetworkChangeNotifierFactory());
  }

  // Creates a SingleThreadTaskExecutor for Android WebView if doesn't exist.
  DCHECK(!main_task_executor_.get());
  if (!base::MessageLoopCurrent::IsSet()) {
    main_task_executor_ = std::make_unique<base::SingleThreadTaskExecutor>(
        base::MessagePumpType::UI);
  }

  browser_process_ = std::make_unique<BisonBrowserProcess>(
      browser_client_->bison_feature_list_creator());
  return service_manager::RESULT_CODE_NORMAL_EXIT;
}

int BisonBrowserMainParts::PreCreateThreads() {
  base::android::MemoryPressureListenerAndroid::Initialize(
      base::android::AttachCurrentThread());
  ::crash_reporter::ChildExitObserver::Create();
  

  // if (base::CommandLine::ForCurrentProcess()->HasSwitch(
  //         switches::kWebViewSandboxedRenderer)) {
  //   // Create the renderers crash manager on the UI thread.
  //   VLOG(0) << "enable kWebViewSandboxedRenderer";
  //   ::crash_reporter::ChildExitObserver::GetInstance()->RegisterClient(
  //       std::make_unique<BisonBrowserTerminator>());
  // }else {
  //   VLOG(0) << "disable kWebViewSandboxedRenderer";
  // }
  ::crash_reporter::ChildExitObserver::GetInstance()->RegisterClient(
          std::make_unique<BisonBrowserTerminator>());
  return service_manager::RESULT_CODE_NORMAL_EXIT;
}

void BisonBrowserMainParts::PreMainMessageLoopRun() {
  BisonBrowserProcess::GetInstance()->PreMainMessageLoopRun();
  browser_client_->InitBrowserContext();
  // content::WebUIControllerFactory::RegisterFactory(
  //     BisonWebUIControllerFactory::GetInstance());
  content::RenderFrameHost::AllowInjectingJavaScript();
  //metrics_logger_ = std::make_unique<metrics::MemoryMetricsLogger>();
}

bool BisonBrowserMainParts::MainMessageLoopRun(int* result_code) {
  return true;
}

void BisonBrowserMainParts::PostCreateThreads() {
  heap_profiling::Mode mode = heap_profiling::GetModeForStartup();
  if (mode != heap_profiling::Mode::kNone)
    heap_profiling::Supervisor::GetInstance()->Start(base::NullCallback());
}

}  // namespace bison
