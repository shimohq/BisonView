#include "bison/browser/bison_browser_main_parts.h"

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop_current.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "bison/browser/bison_browser_context.h"
#include "bison/browser/bison_content_browser_client.h"
#include "bison/browser/bison_contents.h"
#include "bison/browser/bison_devtools_manager_delegate.h"
#include "bison/browser/network_service/bison_network_change_notifier_factory.h"
#include "build/build_config.h"
#include "cc/base/switches.h"
#include "components/crash/content/browser/child_exit_observer_android.h"
#include "components/crash/content/browser/child_process_crash_observer_android.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"
#include "content/public/common/url_constants.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "net/android/network_change_notifier_factory_android.h"
#include "net/base/filename_util.h"
#include "net/base/net_module.h"
#include "net/base/network_change_notifier.h"
#include "net/grit/net_resources.h"
#include "services/service_manager/embedder/result_codes.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

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

  return service_manager::RESULT_CODE_NORMAL_EXIT;
}

void BisonBrowserMainParts::InitializeBrowserContexts() {}

int BisonBrowserMainParts::PreCreateThreads() {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  crash_reporter::ChildExitObserver::Create();
  if (command_line->HasSwitch(switches::kEnableCrashReporter)) {
    crash_reporter::ChildExitObserver::GetInstance()->RegisterClient(
        std::make_unique<crash_reporter::ChildProcessCrashObserver>());
  }
  VLOG(0) << "PreCreateThreads";
  return 0;
}

void BisonBrowserMainParts::PreMainMessageLoopRun() {
  content::RenderFrameHost::AllowInjectingJavaScript();
  InitializeBrowserContexts();
  //jiang 
  browser_client_->InitBrowserContext();
  VLOG(0) << "PreMainMessageLoopRun";
  //net::NetModule::SetResourceProvider(PlatformResourceProvider);
  //BisonDevToolsManagerDelegate::StartHttpHandler(browser_context_.get());

}

bool BisonBrowserMainParts::MainMessageLoopRun(int* result_code) {
  return true;
}

// jiang 运行没有问题后就可以删掉了
// void BisonBrowserMainParts::PreDefaultMainMessageLoopRun(
//     base::OnceClosure quit_closure) {
//   BisonContents::SetMainMessageLoopQuitClosure(std::move(quit_closure));
// }

void BisonBrowserMainParts::PostDestroyThreads() {}

}  // namespace bison
