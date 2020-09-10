// create by jiang947

#ifndef BISON_BROWSER_BISON_CONTENTS_CLIENT_BRIDGE_H_
#define BISON_BROWSER_BISON_CONTENTS_CLIENT_BRIDGE_H_

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/containers/id_map.h"
#include "base/supports_user_data.h"
#include "bison/browser/network_service/bison_web_resource_request.h"
#include "content/public/browser/javascript_dialog_manager.h"
#include "content/public/browser/web_contents.h"

namespace bison {

class BisonContentsClientBridge {
 public:
  struct HttpErrorInfo {
    HttpErrorInfo();
    ~HttpErrorInfo();

    int status_code;
    std::string status_text;
    std::string mime_type;
    std::string encoding;
    std::vector<std::string> response_header_names;
    std::vector<std::string> response_header_values;
  };

  static void Associate(content::WebContents* web_contents,
                        BisonContentsClientBridge* handler);
  static BisonContentsClientBridge* FromWebContents(
      content::WebContents* web_contents);

  BisonContentsClientBridge(JNIEnv* env,
                            const base::android::JavaRef<jobject>& obj);
  ~BisonContentsClientBridge();

  void RunJavaScriptDialog(
      content::JavaScriptDialogType dialog_type,
      const GURL& origin_url,
      const base::string16& message_text,
      const base::string16& default_prompt_text,
      content::JavaScriptDialogManager::DialogClosedCallback callback);

  void ConfirmJsResult(JNIEnv*,
                       const base::android::JavaRef<jobject>&,
                       int id,
                       const base::android::JavaRef<jstring>& prompt);
  void CancelJsResult(JNIEnv*, const base::android::JavaRef<jobject>&, int id);

  bool ShouldOverrideUrlLoading(const base::string16& url,
                                bool has_user_gesture,
                                bool is_redirect,
                                bool is_main_frame,
                                bool* ignore_navigation);

  void OnReceivedHttpError(const BisonWebResourceRequest& request,
                           std::unique_ptr<HttpErrorInfo> error_info);

 private:
  JavaObjectWeakGlobalRef java_ref_;

  base::IDMap<
      std::unique_ptr<content::JavaScriptDialogManager::DialogClosedCallback>>
      pending_js_dialog_callbacks_;
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_CONTENTS_CLIENT_BRIDGE_H_
