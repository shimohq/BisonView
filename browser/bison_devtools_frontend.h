// create by jiang947

#ifndef BISON_BROWSER_BISON_DEVTOOLS_FRONTEND_H_
#define BISON_BROWSER_BISON_DEVTOOLS_FRONTEND_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "bison_devtools_bindings.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
}  // namespace content
using content::WebContents;
using content::WebContentsObserver;

namespace bison {

class BisonContents;

class BisonDevToolsFrontend : public BisonDevToolsDelegate,
                              public WebContentsObserver {
 public:
  static BisonDevToolsFrontend* Show(WebContents* inspected_contents);

  void Activate();
  void Focus();
  void InspectElementAt(int x, int y);
  void Close() override;

  BisonContents* frontend_shell() const { return frontend_bison_view_; }

 private:
  // WebContentsObserver overrides
  void DocumentAvailableInMainFrame() override;
  void WebContentsDestroyed() override;

  BisonDevToolsFrontend(BisonContents* frontend_bison_view,
                        WebContents* inspected_contents);
  ~BisonDevToolsFrontend() override;

  BisonContents* frontend_bison_view_;
  std::unique_ptr<BisonDevToolsBindings> devtools_bindings_;

  DISALLOW_COPY_AND_ASSIGN(BisonDevToolsFrontend);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_DEVTOOLS_FRONTEND_H_
