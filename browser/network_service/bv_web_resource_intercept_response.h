// create by jiang947

#ifndef BISON_BROWSER_NETWORK_SERVICE_BV_WEB_RESOURCE_INTERCEPT_RESPONSE_H_
#define BISON_BROWSER_NETWORK_SERVICE_BV_WEB_RESOURCE_INTERCEPT_RESPONSE_H_

#include <memory>
#include <vector>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/compiler_specific.h"

namespace embedder_support {
class WebResourceResponse;
}

namespace bison {

class BvWebResourceResponse;

class BvWebResourceInterceptResponse {
 public:
  BvWebResourceInterceptResponse() = delete;

  // It is expected that |obj| is an instance of the Java-side
  // org.chromium.bison.BvWebResourceInterceptResponse class.
  explicit BvWebResourceInterceptResponse(
      const base::android::JavaRef<jobject>& obj);

  BvWebResourceInterceptResponse(const BvWebResourceInterceptResponse&) =
      delete;
  BvWebResourceInterceptResponse& operator=(
      const BvWebResourceInterceptResponse&) = delete;

  ~BvWebResourceInterceptResponse();

  // True if the call to shouldInterceptRequest raised an exception.
  bool RaisedException(JNIEnv* env) const;

  // True if this object contains a response.
  bool HasResponse(JNIEnv* env) const;

  // The response returned by the Java-side handler. Caller should first check
  // if an exception was caught via RaisedException() before calling
  // this method. A null value means do not intercept the response.
  std::unique_ptr<embedder_support::WebResourceResponse> GetResponse(
      JNIEnv* env) const;

 private:
  base::android::ScopedJavaGlobalRef<jobject> java_object_;
};

}  // namespace bison

#endif  // BISON_BROWSER_NETWORK_SERVICE_BV_WEB_RESOURCE_INTERCEPT_RESPONSE_H_
