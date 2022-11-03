
#include "bison/browser/bv_browser_main_parts.h"

#include <memory>
#include <set>
#include <string>
#include <utility>

#include "bison/browser/bv_browser_context.h"
#include "bison/browser/bv_browser_terminator.h"
#include "bison/browser/bv_content_browser_client.h"
#include "bison/browser/bv_contents.h"
#include "bison/browser/bv_devtools_manager_delegate.h"
#include "bison/browser/network_service/bv_network_change_notifier_factory.h"
#include "bison/common/bv_paths.h"
#include "bison/common/bv_switches.h"

#include "base/android/apk_assets.h"
#include "base/android/build_info.h"
#include "base/android/bundle_utils.h"
#include "base/android/memory_pressure_listener_android.h"
#include "base/base_paths_android.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/message_loop/message_pump_type.h"
#include "base/path_service.h"
#include "base/task/current_thread.h"
#include "components/crash/content/browser/child_exit_observer_android.h"
#include "components/crash/core/common/crash_key.h"
#include "components/embedder_support/android/metrics/memory_metrics_logger.h"
#include "components/heap_profiling/multi_process/supervisor.h"
#include "components/metrics/metrics_service.h"
#include "components/services/heap_profiling/public/cpp/settings.h"
#include "components/user_prefs/user_prefs.h"
#include "components/variations/synthetic_trials.h"
#include "components/variations/synthetic_trials_active_group_id_provider.h"
#include "components/variations/variations_crash_keys.h"
#include "components/variations/variations_ids_provider.h"
#include "content/public/browser/android/synchronous_compositor.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/result_codes.h"
#include "net/android/network_change_notifier_factory_android.h"
#include "net/base/network_change_notifier.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/layout.h"
#include "ui/gl/gl_surface.h"

namespace bison {

BvBrowserMainParts::BvBrowserMainParts(
    BvContentBrowserClient* browser_client)
    : browser_client_(browser_client) {}

BvBrowserMainParts::~BvBrowserMainParts() {}

int BvBrowserMainParts::PreEarlyInitialization() {
  if (!net::NetworkChangeNotifier::GetFactory()) {
    net::NetworkChangeNotifier::SetFactory(
        new BvNetworkChangeNotifierFactory());
  }

  // Creates a SingleThreadTaskExecutor for Android WebView if doesn't exist.
  DCHECK(!main_task_executor_.get());
  if (!base::CurrentThread::IsSet()) {
    main_task_executor_ = std::make_unique<base::SingleThreadTaskExecutor>(
        base::MessagePumpType::UI);
  }

  browser_process_ = std::make_unique<BvBrowserProcess>(
      browser_client_->bv_feature_list_creator());
  return content::RESULT_CODE_NORMAL_EXIT;
}

int BvBrowserMainParts::PreCreateThreads() {
  base::android::MemoryPressureListenerAndroid::Initialize(
      base::android::AttachCurrentThread());
  child_exit_observer_ =
      std::make_unique<::crash_reporter::ChildExitObserver>();

  // We need to create the safe browsing specific directory even if the
  // AwSafeBrowsingConfigHelper::GetSafeBrowsingEnabled() is false
  // initially, because safe browsing can be enabled later at runtime
  // on a per-webview basis.

  base::FilePath crash_dir;
  if (base::PathService::Get(bison::DIR_CRASH_DUMPS, &crash_dir)) {
    if (!base::PathExists(crash_dir)) {
      base::CreateDirectory(crash_dir);
    }
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kWebViewSandboxedRenderer)) {
    // Create the renderers crash manager on the UI thread.
    child_exit_observer_->RegisterClient(
        std::make_unique<BvBrowserTerminator>());
  }

  crash_reporter::InitializeCrashKeys();
  variations::InitCrashKeys();

  RegisterSyntheticTrials();

  return content::RESULT_CODE_NORMAL_EXIT;
}

void BvBrowserMainParts::RegisterSyntheticTrials() {
  // jiang
  // metrics::MetricsService* metrics =
  //     AwMetricsServiceClient::GetInstance()->GetMetricsService();
  // metrics->GetSyntheticTrialRegistry()->AddSyntheticTrialObserver(
  //     variations::VariationsIdsProvider::GetInstance());
  // metrics->GetSyntheticTrialRegistry()->AddSyntheticTrialObserver(
  //     variations::SyntheticTrialsActiveGroupIdProvider::GetInstance());

}

int BvBrowserMainParts::PreMainMessageLoopRun() {
  BvBrowserProcess::GetInstance()->PreMainMessageLoopRun();
  browser_client_->InitBrowserContext();
  content::RenderFrameHost::AllowInjectingJavaScript();
  return content::RESULT_CODE_NORMAL_EXIT;
}

void BvBrowserMainParts::WillRunMainMessageLoop(
    std::unique_ptr<base::RunLoop>& run_loop) {
  NOTREACHED();
}

void BvBrowserMainParts::PostCreateThreads() {
  heap_profiling::Mode mode = heap_profiling::GetModeForStartup();
  if (mode != heap_profiling::Mode::kNone)
    heap_profiling::Supervisor::GetInstance()->Start(base::NullCallback());
}

}  // namespace bison
