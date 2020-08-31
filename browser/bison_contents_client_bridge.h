

// create by jiang947

#ifndef BISON_BROWSER_BISON_CONTENTS_CLIENT_BRIDGE_H_
#define BISON_BROWSER_BISON_CONTENTS_CLIENT_BRIDGE_H_

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/containers/id_map.h"
#include "base/supports_user_data.h"
#include "content/public/browser/javascript_dialog_manager.h"
#include "content/public/browser/web_contents.h"

namespace bison {

class BisonContentsClientBridge {
 public:
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

 private:
  JavaObjectWeakGlobalRef java_ref_;

  base::IDMap<
      std::unique_ptr<content::JavaScriptDialogManager::DialogClosedCallback>>
      pending_js_dialog_callbacks_;
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_CONTENTS_CLIENT_BRIDGE_H_
