// create by jiang947

#ifndef BISON_BROWSER_BISON_BROWSER_MAIN_PARTS_H_
#define BISON_BROWSER_BISON_BROWSER_MAIN_PARTS_H_

#include <memory>

#include "bison/browser/bison_browser_context.h"
#include "bison/browser/bison_browser_process.h"

#include "base/macros.h"
#include "base/task/single_thread_task_executor.h"
#include "content/public/browser/browser_main_parts.h"

namespace bison {

class BisonBrowserProcess;
class BisonContentBrowserClient;
class MemoryMetricsLogger;

class BisonBrowserMainParts : public content::BrowserMainParts {
 public:
  explicit BisonBrowserMainParts(BisonContentBrowserClient* browser_client);
  ~BisonBrowserMainParts() override;

  // BrowserMainParts overrides.
  int PreEarlyInitialization() override;
  int PreCreateThreads() override;
  void PreMainMessageLoopRun() override;
  bool MainMessageLoopRun(int* result_code) override;
  void PostCreateThreads() override;

 private:
  std::unique_ptr<base::SingleThreadTaskExecutor> main_task_executor_;

  BisonContentBrowserClient* browser_client_;

  std::unique_ptr<MemoryMetricsLogger> metrics_logger_;

  std::unique_ptr<BisonBrowserProcess> browser_process_;

  DISALLOW_COPY_AND_ASSIGN(BisonBrowserMainParts);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_BROWSER_MAIN_PARTS_H_
