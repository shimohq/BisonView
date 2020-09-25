// create by jiang947

#ifndef BISON_BROWSER_BISON_DEVTOOLS_MANAGER_DELEGATE_H_
#define BISON_BROWSER_BISON_DEVTOOLS_MANAGER_DELEGATE_H_

#include <jni.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "content/public/browser/devtools_manager_delegate.h"

namespace bison {

// Delegate implementation for the devtools http handler for WebView. A new
// instance of this gets created each time web debugging is enabled.
class BisonDevToolsManagerDelegate : public content::DevToolsManagerDelegate {
 public:
  BisonDevToolsManagerDelegate();
  ~BisonDevToolsManagerDelegate() override;

  // content::DevToolsManagerDelegate implementation.
  std::string GetTargetDescription(content::WebContents* web_contents) override;
  std::string GetDiscoveryPageHTML() override;
  bool IsBrowserTargetDiscoverable() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BisonDevToolsManagerDelegate);
};

}  //  namespace bison

#endif  // BISON_BROWSER_BISON_DEVTOOLS_MANAGER_DELEGATE_H_
