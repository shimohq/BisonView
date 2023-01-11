// create by jiang947

#ifndef BISON_BROWSER_BISON_JAVASCRIPT_DIALOG_MANAGER_H_
#define BISON_BROWSER_BISON_JAVASCRIPT_DIALOG_MANAGER_H_

#include <memory>

#include "content/public/browser/javascript_dialog_manager.h"

namespace bison {

class BvJavaScriptDialogManager : public content::JavaScriptDialogManager {
 public:
  BvJavaScriptDialogManager();
  BvJavaScriptDialogManager(const BvJavaScriptDialogManager&) = delete;
  BvJavaScriptDialogManager& operator=(const BvJavaScriptDialogManager&) =
      delete;

  ~BvJavaScriptDialogManager() override;

  // Overridden from content::JavaScriptDialogManager:
  void RunJavaScriptDialog(content::WebContents* web_contents,
                           content::RenderFrameHost* render_frame_host,
                           content::JavaScriptDialogType dialog_type,
                           const std::u16string& message_text,
                           const std::u16string& default_prompt_text,
                           DialogClosedCallback callback,
                           bool* did_suppress_message) override;
  void RunBeforeUnloadDialog(content::WebContents* web_contents,
                             content::RenderFrameHost* render_frame_host,
                             bool is_reload,
                             DialogClosedCallback callback) override;
  void CancelDialogs(content::WebContents* web_contents,
                     bool reset_state) override;
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_JAVASCRIPT_DIALOG_MANAGER_H_
