// create by jiang947

#ifndef BISON_BROWSER_BISON_BROWSER_MAIN_PARTS_H_
#define BISON_BROWSER_BISON_BROWSER_MAIN_PARTS_H_

#include <memory>

#include "bison/browser/bv_browser_context.h"
#include "bison/browser/bv_browser_process.h"

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/task/single_thread_task_executor.h"
#include "content/public/browser/browser_main_parts.h"

namespace metrics {
class MemoryMetricsLogger;
}

namespace bison {

class BvBrowserProcess;
class BvContentBrowserClient;

class BvBrowserMainParts : public content::BrowserMainParts {
 public:
  explicit BvBrowserMainParts(BvContentBrowserClient* browser_client);
  ~BvBrowserMainParts() override;

  // BrowserMainParts overrides.
  int PreEarlyInitialization() override;
  int PreCreateThreads() override;
  void PreMainMessageLoopRun() override;
  bool MainMessageLoopRun(int* result_code) override;
  void PostCreateThreads() override;

 private:
  std::unique_ptr<base::SingleThreadTaskExecutor> main_task_executor_;

  BvContentBrowserClient* browser_client_;

  //std::unique_ptr<metrics::MemoryMetricsLogger> metrics_logger_;

  std::unique_ptr<BvBrowserProcess> browser_process_;

  DISALLOW_COPY_AND_ASSIGN(BvBrowserMainParts);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_BROWSER_MAIN_PARTS_H_
