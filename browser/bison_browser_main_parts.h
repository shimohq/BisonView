// create by jiang947

#ifndef BISON_BROWSER_BISON_BROWSER_MAIN_PARTS_H_
#define BISON_BROWSER_BISON_BROWSER_MAIN_PARTS_H_

#include <memory>

#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "bison/browser/bison_browser_context.h"
#include "build/build_config.h"
#include "content/public/browser/browser_main_parts.h"
#include "content/public/common/main_function_params.h"

using content::MainFunctionParams;

namespace bison {

class BisonBrowserMainParts : public content::BrowserMainParts {
 public:
  explicit BisonBrowserMainParts(const MainFunctionParams& parameters);
  ~BisonBrowserMainParts() override;

  // BrowserMainParts overrides.
  int PreEarlyInitialization() override;
  int PreCreateThreads() override;
  void PreMainMessageLoopStart() override;
  void PostMainMessageLoopStart() override;
  void PreMainMessageLoopRun() override;
  bool MainMessageLoopRun(int* result_code) override;
  void PreDefaultMainMessageLoopRun(base::OnceClosure quit_closure) override;
  void PostMainMessageLoopRun() override;
  void PostDestroyThreads() override;

  BisonBrowserContext* browser_context() { return browser_context_.get(); }
  BisonBrowserContext* off_the_record_browser_context() {
    return off_the_record_browser_context_.get();
  }

 protected:
  virtual void InitializeBrowserContexts();
  virtual void InitializeMessageLoopContext();

  void set_browser_context(BisonBrowserContext* context) {
    browser_context_.reset(context);
  }
  void set_off_the_record_browser_context(BisonBrowserContext* context) {
    off_the_record_browser_context_.reset(context);
  }

 private:
  std::unique_ptr<BisonBrowserContext> browser_context_;
  std::unique_ptr<BisonBrowserContext> off_the_record_browser_context_;

  // For running content_browsertests.
  const MainFunctionParams parameters_;
  bool run_message_loop_;

  DISALLOW_COPY_AND_ASSIGN(BisonBrowserMainParts);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_BROWSER_MAIN_PARTS_H_
