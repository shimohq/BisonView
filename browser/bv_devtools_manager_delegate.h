// create by jiang947

#ifndef BISON_BROWSER_BISON_DEVTOOLS_MANAGER_DELEGATE_H_
#define BISON_BROWSER_BISON_DEVTOOLS_MANAGER_DELEGATE_H_

#include <jni.h>

#include <memory>
#include <vector>

#include "content/public/browser/devtools_manager_delegate.h"

namespace bison {

// Delegate implementation for the devtools http handler for WebView. A new
// instance of this gets created each time web debugging is enabled.
class BvDevToolsManagerDelegate : public content::DevToolsManagerDelegate {
 public:
  BvDevToolsManagerDelegate();
  BvDevToolsManagerDelegate(const BvDevToolsManagerDelegate&) = delete;
  BvDevToolsManagerDelegate& operator=(const BvDevToolsManagerDelegate&) =
      delete;

  ~BvDevToolsManagerDelegate() override;

  // content::DevToolsManagerDelegate implementation.
  std::string GetTargetDescription(content::WebContents* web_contents) override;
  std::string GetDiscoveryPageHTML() override;
  bool IsBrowserTargetDiscoverable() override;
};

}  //  namespace bison

#endif  // BISON_BROWSER_BISON_DEVTOOLS_MANAGER_DELEGATE_H_
