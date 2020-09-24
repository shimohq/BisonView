// create by jiang947

#ifndef BISON_BROWSER_BISON_DEVTOOLS_MANAGER_DELEGATE_H_
#define BISON_BROWSER_BISON_DEVTOOLS_MANAGER_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "content/public/browser/devtools_manager_delegate.h"

namespace content {
class BrowserContext;
}

using content::BrowserContext;
using content::DevToolsAgentHost;
using content::DevToolsManagerDelegate;

namespace bison {

class BisonDevToolsManagerDelegate : public DevToolsManagerDelegate {
 public:
  BisonDevToolsManagerDelegate();
  ~BisonDevToolsManagerDelegate() override;


  static void StartHttpHandler(BrowserContext* browser_context);
  static void StopHttpHandler();
  static int GetHttpHandlerPort();

  // DevToolsManagerDelegate implementation.
  std::string GetTargetDescription(content::WebContents* web_contents) override;
  std::string GetDiscoveryPageHTML() override;
  bool IsBrowserTargetDiscoverable() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BisonDevToolsManagerDelegate);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_DEVTOOLS_MANAGER_DELEGATE_H_
