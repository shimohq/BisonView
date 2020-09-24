// create by jiang947

#ifndef BISON_BROWSER_BISON_BROWSER_MAIN_PARTS_H_
#define BISON_BROWSER_BISON_BROWSER_MAIN_PARTS_H_

#include <memory>

#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/task/single_thread_task_executor.h"
#include "bison/browser/bison_browser_context.h"
#include "content/public/browser/browser_main_parts.h"




namespace bison {

class BisonContentBrowserClient;

class BisonBrowserMainParts : public content::BrowserMainParts {
 public:
  explicit BisonBrowserMainParts(BisonContentBrowserClient* browser_client);
  ~BisonBrowserMainParts() override;

  // BrowserMainParts overrides.
  int PreEarlyInitialization() override;
  int PreCreateThreads() override;
  void PreMainMessageLoopRun() override;
  bool MainMessageLoopRun(int* result_code) override;
  void PostDestroyThreads() override;

  //BisonBrowserContext* browser_context() { return browser_context_.get(); }

 protected:
  virtual void InitializeBrowserContexts();

 private:
  std::unique_ptr<base::SingleThreadTaskExecutor> main_task_executor_;

  BisonContentBrowserClient* browser_client_;
  // std::unique_ptr<BisonBrowserContext> browser_context_;
  

  DISALLOW_COPY_AND_ASSIGN(BisonBrowserMainParts);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_BROWSER_MAIN_PARTS_H_
