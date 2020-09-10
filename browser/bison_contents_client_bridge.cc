// create by jiang947
#include "bison_contents_client_bridge.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/message_loop/message_loop_current.h"
#include "bison/bison_jni_headers/BisonContentsClientBridge_jni.h"
#include "content/public/browser/browser_thread.h"

using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF16;
using base::android::ConvertUTF16ToJavaString;
using base::android::ConvertUTF8ToJavaString;
using base::android::HasException;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;
using content::WebContents;

namespace bison {

namespace {

const void* const kBisonContentsClientBridge = &kBisonContentsClientBridge;

class UserData : public base::SupportsUserData::Data {
 public:
  static BisonContentsClientBridge* GetContents(
      content::WebContents* web_contents) {
    if (!web_contents)
      return NULL;
    UserData* data = static_cast<UserData*>(
        web_contents->GetUserData(kBisonContentsClientBridge));
    return data ? data->contents_ : NULL;
  }

  explicit UserData(BisonContentsClientBridge* ptr) : contents_(ptr) {}

 private:
  BisonContentsClientBridge* contents_;

  DISALLOW_COPY_AND_ASSIGN(UserData);
};

}  // namespace

// static
void BisonContentsClientBridge::Associate(WebContents* web_contents,
                                          BisonContentsClientBridge* handler) {
  web_contents->SetUserData(kBisonContentsClientBridge,
                            std::make_unique<UserData>(handler));
}

BisonContentsClientBridge* BisonContentsClientBridge::FromWebContents(
    WebContents* web_contents) {
  return UserData::GetContents(web_contents);
}
// end static

BisonContentsClientBridge::BisonContentsClientBridge(
    JNIEnv* env,
    const JavaRef<jobject>& obj)
    : java_ref_(env, obj) {
  DCHECK(!obj.is_null());
  Java_BisonContentsClientBridge_setNativeContentsClientBridge(
      env, obj, reinterpret_cast<intptr_t>(this));
}

BisonContentsClientBridge::~BisonContentsClientBridge() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (!obj.is_null()) {
    Java_BisonContentsClientBridge_setNativeContentsClientBridge(env, obj, 0);
  }
}

void BisonContentsClientBridge::RunJavaScriptDialog(
    content::JavaScriptDialogType dialog_type,
    const GURL& origin_url,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    content::JavaScriptDialogManager::DialogClosedCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null()) {
    std::move(callback).Run(false, base::string16());
    return;
  }

  int callback_id = pending_js_dialog_callbacks_.Add(
      std::make_unique<content::JavaScriptDialogManager::DialogClosedCallback>(
          std::move(callback)));
  ScopedJavaLocalRef<jstring> jurl(
      ConvertUTF8ToJavaString(env, origin_url.spec()));
  ScopedJavaLocalRef<jstring> jmessage(
      ConvertUTF16ToJavaString(env, message_text));

  switch (dialog_type) {
    case content::JAVASCRIPT_DIALOG_TYPE_ALERT: {
      // devtools_instrumentation::ScopedEmbedderCallbackTask("onJsAlert");
      Java_BisonContentsClientBridge_handleJsAlert(env, obj, jurl, jmessage,
                                                   callback_id);
      break;
    }
    case content::JAVASCRIPT_DIALOG_TYPE_CONFIRM: {
      // devtools_instrumentation::ScopedEmbedderCallbackTask("onJsConfirm");
      VLOG(0) << "onJsConfirm";
      Java_BisonContentsClientBridge_handleJsConfirm(env, obj, jurl, jmessage,
                                                     callback_id);
      break;
    }
    case content::JAVASCRIPT_DIALOG_TYPE_PROMPT: {
      ScopedJavaLocalRef<jstring> jdefault_value(
          ConvertUTF16ToJavaString(env, default_prompt_text));
      // devtools_instrumentation::ScopedEmbedderCallbackTask("onJsPrompt");
      Java_BisonContentsClientBridge_handleJsPrompt(
          env, obj, jurl, jmessage, jdefault_value, callback_id);
      break;
    }
    default:
      NOTREACHED();
  }
}

void BisonContentsClientBridge::ConfirmJsResult(
    JNIEnv* env,
    const JavaRef<jobject>&,
    int id,
    const JavaRef<jstring>& prompt) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::JavaScriptDialogManager::DialogClosedCallback* callback =
      pending_js_dialog_callbacks_.Lookup(id);
  if (!callback) {
    LOG(WARNING) << "Unexpected JS dialog confirm. " << id;
    return;
  }
  base::string16 prompt_text;
  if (!prompt.is_null()) {
    prompt_text = ConvertJavaStringToUTF16(env, prompt);
  }
  std::move(*callback).Run(true, prompt_text);
  pending_js_dialog_callbacks_.Remove(id);
}

void BisonContentsClientBridge::CancelJsResult(JNIEnv*,
                                               const JavaRef<jobject>&,
                                               int id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::JavaScriptDialogManager::DialogClosedCallback* callback =
      pending_js_dialog_callbacks_.Lookup(id);
  if (!callback) {
    LOG(WARNING) << "Unexpected JS dialog cancel. " << id;
    return;
  }
  std::move(*callback).Run(false, base::string16());
  pending_js_dialog_callbacks_.Remove(id);
}

bool BisonContentsClientBridge::ShouldOverrideUrlLoading(
    const base::string16& url,
    bool has_user_gesture,
    bool is_redirect,
    bool is_main_frame,
    bool* ignore_navigation) {
  *ignore_navigation = false;
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return true;
  ScopedJavaLocalRef<jstring> jurl = ConvertUTF16ToJavaString(env, url);
  // devtools_instrumentation::ScopedEmbedderCallbackTask(
  //     "shouldOverrideUrlLoading");
  *ignore_navigation = Java_BisonContentsClientBridge_shouldOverrideUrlLoading(
      env, obj, jurl, has_user_gesture, is_redirect, is_main_frame);
  if (HasException(env)) {
    // Tell the chromium message loop to not perform any tasks after the current
    // one - we want to make sure we return to Java cleanly without first making
    // any new JNI calls.
    base::MessageLoopCurrentForUI::Get()->Abort();
    // If we crashed we don't want to continue the navigation.
    *ignore_navigation = true;
    return false;
  }
  return true;
}

void BisonContentsClientBridge::OnReceivedHttpError(
    const BisonWebResourceRequest& request,
    std::unique_ptr<HttpErrorInfo> http_error_info) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  VLOG(0) << "OnReceivedHttpError";
  // JNIEnv* env = AttachCurrentThread();
  // ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  // if (obj.is_null())
  //   return;

  // AwWebResourceRequest::AwJavaWebResourceRequest java_web_resource_request;
  // AwWebResourceRequest::ConvertToJava(env, request,
  // &java_web_resource_request);

  // ScopedJavaLocalRef<jstring> jstring_mime_type =
  //     ConvertUTF8ToJavaString(env, http_error_info->mime_type);
  // ScopedJavaLocalRef<jstring> jstring_encoding =
  //     ConvertUTF8ToJavaString(env, http_error_info->encoding);
  // ScopedJavaLocalRef<jstring> jstring_reason =
  //     ConvertUTF8ToJavaString(env, http_error_info->status_text);
  // ScopedJavaLocalRef<jobjectArray> jstringArray_response_header_names =
  //     ToJavaArrayOfStrings(env, http_error_info->response_header_names);
  // ScopedJavaLocalRef<jobjectArray> jstringArray_response_header_values =
  //     ToJavaArrayOfStrings(env, http_error_info->response_header_values);

  // Java_AwContentsClientBridge_onReceivedHttpError(
  //     env, obj, java_web_resource_request.jurl, request.is_main_frame,
  //     request.has_user_gesture, java_web_resource_request.jmethod,
  //     java_web_resource_request.jheader_names,
  //     java_web_resource_request.jheader_values, jstring_mime_type,
  //     jstring_encoding, http_error_info->status_code, jstring_reason,
  //     jstringArray_response_header_names,
  //     jstringArray_response_header_values);
  }

}  // namespace bison