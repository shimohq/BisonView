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
  static void StartHttpHandler(BrowserContext* browser_context);
  static void StopHttpHandler();
  static int GetHttpHandlerPort();

  explicit BisonDevToolsManagerDelegate(BrowserContext* browser_context);
  ~BisonDevToolsManagerDelegate() override;

  // DevToolsManagerDelegate implementation.
  BrowserContext* GetDefaultBrowserContext() override;
  scoped_refptr<DevToolsAgentHost> CreateNewTarget(const GURL& url) override;
  std::string GetDiscoveryPageHTML() override;
  bool HasBundledFrontendResources() override;
  void ClientAttached(content::DevToolsAgentHost* agent_host,
                      content::DevToolsAgentHostClient* client) override;
  void ClientDetached(content::DevToolsAgentHost* agent_host,
                      content::DevToolsAgentHostClient* client) override;

 private:
  BrowserContext* browser_context_;
  base::flat_set<content::DevToolsAgentHostClient*> clients_;
  DISALLOW_COPY_AND_ASSIGN(BisonDevToolsManagerDelegate);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_DEVTOOLS_MANAGER_DELEGATE_H_
