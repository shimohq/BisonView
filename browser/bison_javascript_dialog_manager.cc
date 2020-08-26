#include "bison_javascript_dialog_manager.h"

// #include "android_webview/browser/aw_contents_client_bridge.h"
#include "content/public/browser/javascript_dialog_manager.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace bison {

BisonJavaScriptDialogManager::BisonJavaScriptDialogManager() {}

BisonJavaScriptDialogManager::~BisonJavaScriptDialogManager() {}

void BisonJavaScriptDialogManager::RunJavaScriptDialog(
    WebContents* web_contents,
    RenderFrameHost* render_frame_host,
    JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    DialogClosedCallback callback,
    bool* did_suppress_message) {}

void BisonJavaScriptDialogManager::RunBeforeUnloadDialog(
    WebContents* web_contents,
    RenderFrameHost* render_frame_host,
    bool is_reload,
    DialogClosedCallback callback) {}

void BisonJavaScriptDialogManager::CancelDialogs(WebContents* web_contents,
                                                 bool reset_state) {}

}  // namespace bison
