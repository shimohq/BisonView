// create by jiang947

#ifndef BISON_BROWSER_NETWORK_SERVICE_BISON_WEB_RESOURCE_INTERCEPT_RESPONSE_H_
#define BISON_BROWSER_NETWORK_SERVICE_BISON_WEB_RESOURCE_INTERCEPT_RESPONSE_H_

#include <memory>
#include <vector>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/compiler_specific.h"
#include "base/macros.h"

namespace bison {

class BisonWebResourceResponse;

class BisonWebResourceInterceptResponse {
 public:
  BisonWebResourceInterceptResponse() = delete;

  // It is expected that |obj| is an instance of the Java-side
  // org.chromium.bison.BisonWebResourceInterceptResponse class.
  explicit BisonWebResourceInterceptResponse(
      const base::android::JavaRef<jobject>& obj);
  ~BisonWebResourceInterceptResponse();

  // True if the call to shouldInterceptRequest raised an exception.
  bool RaisedException(JNIEnv* env) const;

  // True if this object contains a response.
  bool HasResponse(JNIEnv* env) const;

  // The response returned by the Java-side handler. Caller should first check
  // if an exception was caught via RaisedException() before calling
  // this method. A null value means do not intercept the response.
  std::unique_ptr<BisonWebResourceResponse> GetResponse(JNIEnv* env) const;

 private:
  base::android::ScopedJavaGlobalRef<jobject> java_object_;

  DISALLOW_COPY_AND_ASSIGN(BisonWebResourceInterceptResponse);
};

}  // namespace bison

#endif  // BISON_BROWSER_NETWORK_SERVICE_BISON_WEB_RESOURCE_INTERCEPT_RESPONSE_H_
