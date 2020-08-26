// create by jiang947

#ifndef BISON_BROWSER_BISON_JAVASCRIPT_DIALOG_H_
#define BISON_BROWSER_BISON_JAVASCRIPT_DIALOG_H_

#include "base/callback.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "content/public/browser/javascript_dialog_manager.h"

namespace bison {

class BisonJavaScriptDialogManager;

class BisonJavaScriptDialog {
 public:
  BisonJavaScriptDialog(BisonJavaScriptDialogManager* manager,
                        gfx::NativeWindow parent_window,
                        JavaScriptDialogType dialog_type,
                        const base::string16& message_text,
                        const base::string16& default_prompt_text,
                        JavaScriptDialogManager::DialogClosedCallback callback);
  ~BisonJavaScriptDialog();

  // Called to cancel a dialog mid-flight.
  void Cancel();

 private:
  JavaScriptDialogManager::DialogClosedCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(BisonJavaScriptDialog);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_JAVASCRIPT_DIALOG_H_
