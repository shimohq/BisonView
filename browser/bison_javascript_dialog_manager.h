// create by jiang947

#ifndef BISON_BROWSER_BISON_JAVASCRIPT_DIALOG_MANAGER_H_
#define BISON_BROWSER_BISON_JAVASCRIPT_DIALOG_MANAGER_H_

#include <memory>

#include "base/macros.h"
#include "content/public/browser/javascript_dialog_manager.h"

using content::JavaScriptDialogManager;
using content::JavaScriptDialogType;
using content::RenderFrameHost;
using content::WebContents;

namespace bison {

// class BisonJavaScriptDialog;

class BisonJavaScriptDialogManager : public JavaScriptDialogManager {
 public:
  BisonJavaScriptDialogManager();
  ~BisonJavaScriptDialogManager() override;

  void RunJavaScriptDialog(WebContents* web_contents,
                           RenderFrameHost* render_frame_host,
                           JavaScriptDialogType dialog_type,
                           const base::string16& message_text,
                           const base::string16& default_prompt_text,
                           DialogClosedCallback callback,
                           bool* did_suppress_message) override;

  void RunBeforeUnloadDialog(WebContents* web_contents,
                             RenderFrameHost* render_frame_host,
                             bool is_reload,
                             DialogClosedCallback callback) override;

  void CancelDialogs(WebContents* web_contents, bool reset_state) override;

  private:
  DISALLOW_COPY_AND_ASSIGN(BisonJavaScriptDialogManager);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_JAVASCRIPT_DIALOG_MANAGER_H_
