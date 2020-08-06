// create by jiang947

#ifndef BISON_LIB_BISON_MAIN_DELEGATE_H_
#define BISON_LIB_BISON_MAIN_DELEGATE_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "content/public/app/content_main_delegate.h"

namespace content {
class ContentClient;

}  // namespace content

using content::ContentBrowserClient;
using content::ContentClient;
using content::ContentGpuClient;
using content::ContentMainDelegate;
using content::ContentRendererClient;
using content::MainFunctionParams;

namespace bison {
class BisonContentBrowserClient;
class BisonContentGpuClient;
class BisonContentRendererClient;

class BisonMainDelegate : public ContentMainDelegate {
 public:
  explicit BisonMainDelegate();
  ~BisonMainDelegate() override;

  // ContentMainDelegate implementation:
  bool BasicStartupComplete(int* exit_code) override;
  void PreSandboxStartup() override;
  int RunProcess(const std::string& process_type,
                 const MainFunctionParams& main_function_params) override;

  void PreCreateMainMessageLoop() override;
  ContentBrowserClient* CreateContentBrowserClient() override;
  ContentGpuClient* CreateContentGpuClient() override;
  ContentRendererClient* CreateContentRendererClient() override;

  static void InitializeResourceBundle();

 private:
  std::unique_ptr<BisonContentBrowserClient> browser_client_;
  std::unique_ptr<BisonContentGpuClient> gpu_client_;
  std::unique_ptr<BisonContentRendererClient> renderer_client_;

  std::unique_ptr<ContentClient> content_client_;

  DISALLOW_COPY_AND_ASSIGN(BisonMainDelegate);
};

}  // namespace bison

#endif  // BISON_LIB_BISON_MAIN_DELEGATE_H_
