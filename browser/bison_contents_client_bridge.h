// create by jiang947

#ifndef BISON_BROWSER_BISON_CONTENTS_CLIENT_BRIDGE_H_
#define BISON_BROWSER_BISON_CONTENTS_CLIENT_BRIDGE_H_

#include "bison/browser/network_service/bison_web_resource_request.h"

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/callback.h"
#include "base/containers/id_map.h"
#include "base/supports_user_data.h"
#include "components/security_interstitials/content/unsafe_resource.h"
#include "content/public/browser/certificate_request_result_type.h"
#include "content/public/browser/javascript_dialog_manager.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_response_headers.h"

class GURL;

namespace content {
class ClientCertificateDelegate;
class WebContents;
}  // namespace content

namespace net {
class SSLCertRequestInfo;
class X509Certificate;
}  // namespace net

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

  using CertErrorCallback =
      base::OnceCallback<void(content::CertificateRequestResultType)>;

  static void Associate(content::WebContents* web_contents,
                        BisonContentsClientBridge* handler);
  static BisonContentsClientBridge* FromWebContents(
      content::WebContents* web_contents);

  BisonContentsClientBridge(JNIEnv* env,
                            const base::android::JavaRef<jobject>& obj);
  ~BisonContentsClientBridge();

  void AllowCertificateError(int cert_error,
                             net::X509Certificate* cert,
                             const GURL& request_url,
                             CertErrorCallback callback,
                             bool* cancel_request);

  void RunJavaScriptDialog(
      content::JavaScriptDialogType dialog_type,
      const GURL& origin_url,
      const base::string16& message_text,
      const base::string16& default_prompt_text,
      content::JavaScriptDialogManager::DialogClosedCallback callback);
  void RunBeforeUnloadDialog(
      const GURL& origin_url,
      content::JavaScriptDialogManager::DialogClosedCallback callback);
  void ProceedSslError(JNIEnv* env,
                       const base::android::JavaRef<jobject>& obj,
                       jboolean proceed,
                       jint id);

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
  void ProvideClientCertificateResponse(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& object,
      jint request_id,
      const base::android::JavaRef<jobjectArray>& encoded_chain_ref,
      const base::android::JavaRef<jobject>& private_key_ref);

  void OnReceivedHttpError(const BisonWebResourceRequest& request,
                           std::unique_ptr<HttpErrorInfo> error_info);

  static std::unique_ptr<HttpErrorInfo> ExtractHttpErrorInfo(
      const net::HttpResponseHeaders* response_headers);

 private:
  JavaObjectWeakGlobalRef java_ref_;

  base::IDMap<std::unique_ptr<CertErrorCallback>> pending_cert_error_callbacks_;

  base::IDMap<
      std::unique_ptr<content::JavaScriptDialogManager::DialogClosedCallback>>
      pending_js_dialog_callbacks_;
  base::IDMap<std::unique_ptr<content::ClientCertificateDelegate>>
      pending_client_cert_request_delegates_;
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_CONTENTS_CLIENT_BRIDGE_H_
