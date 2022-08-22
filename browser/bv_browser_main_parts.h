// create by jiang947

#ifndef BISON_BROWSER_BISON_BROWSER_MAIN_PARTS_H_
#define BISON_BROWSER_BISON_BROWSER_MAIN_PARTS_H_

#include <memory>

#include "bison/browser/bv_browser_context.h"
#include "bison/browser/bv_browser_process.h"

#include "base/compiler_specific.h"
#include "base/memory/raw_ptr.h"
#include "base/task/single_thread_task_executor.h"
#include "content/public/browser/browser_main_parts.h"

namespace crash_reporter {
class ChildExitObserver;
}

namespace metrics {
class MemoryMetricsLogger;
}

namespace bison {

class BvBrowserProcess;
class BvContentBrowserClient;

class BvBrowserMainParts : public content::BrowserMainParts {
 public:
  explicit BvBrowserMainParts(BvContentBrowserClient* browser_client);

  BvBrowserMainParts(const BvBrowserMainParts&) = delete;
  BvBrowserMainParts& operator=(const BvBrowserMainParts&) = delete;

  ~BvBrowserMainParts() override;

  // BrowserMainParts overrides.
  int PreEarlyInitialization() override;
  int PreCreateThreads() override;
  int PreMainMessageLoopRun() override;
  void WillRunMainMessageLoop(
      std::unique_ptr<base::RunLoop>& run_loop) override;
  void PostCreateThreads() override;

 private:
  void RegisterSyntheticTrials();

  // Android specific UI SingleThreadTaskExecutor.
  std::unique_ptr<base::SingleThreadTaskExecutor> main_task_executor_;

  raw_ptr<BvContentBrowserClient> browser_client_;

  // std::unique_ptr<metrics::MemoryMetricsLogger> metrics_logger_;

  std::unique_ptr<BvBrowserProcess> browser_process_;
  std::unique_ptr<crash_reporter::ChildExitObserver> child_exit_observer_;
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_BROWSER_MAIN_PARTS_H_
