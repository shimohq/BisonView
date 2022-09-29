// create by jiang947

#ifndef BISON_LIB_BISON_MAIN_DELEGATE_H_
#define BISON_LIB_BISON_MAIN_DELEGATE_H_

#include <memory>

#include "bison/browser/bv_feature_list_creator.h"
#include "bison/common/bv_content_client.h"

#include "base/memory/ref_counted.h"
#include "content/public/app/content_main_delegate.h"

using content::ContentBrowserClient;
using content::ContentClient;
using content::ContentGpuClient;
using content::ContentMainDelegate;
using content::ContentRendererClient;
using content::MainFunctionParams;

namespace content {
class BrowserMainRunner;
}
namespace bison {
class BvContentBrowserClient;
class BvContentGpuClient;
class BvContentRendererClient;

class BvMainDelegate : public ContentMainDelegate {
 public:
  explicit BvMainDelegate();

  BvMainDelegate(const BvMainDelegate&) = delete;
  BvMainDelegate& operator=(const BvMainDelegate&) = delete;

  ~BvMainDelegate() override;

 private:
  // ContentMainDelegate implementation:
  bool BasicStartupComplete(int* exit_code) override;
  void PreSandboxStartup() override;
  absl::variant<int, content::MainFunctionParams> RunProcess(
      const std::string& process_type,
      content::MainFunctionParams main_function_params) override;
  void ProcessExiting(const std::string& process_type) override;
  bool ShouldCreateFeatureList() override;
  void PostEarlyInitialization(bool is_running_tests) override;
  void PostFieldTrialInitialization() override;
  content::ContentClient* CreateContentClient() override;
  ContentBrowserClient* CreateContentBrowserClient() override;
  ContentGpuClient* CreateContentGpuClient() override;
  ContentRendererClient* CreateContentRendererClient() override;

 private:
  std::unique_ptr<BvFeatureListCreator> bv_feature_list_creator_;
  std::unique_ptr<BvContentBrowserClient> browser_client_;
  std::unique_ptr<BvContentGpuClient> gpu_client_;
  std::unique_ptr<BvContentRendererClient> renderer_client_;

  std::unique_ptr<content::BrowserMainRunner> browser_runner_;
  BvContentClient content_client_;
};

}  // namespace bison

#endif  // BISON_LIB_BISON_MAIN_DELEGATE_H_
