// create by jiang947

#ifndef BISON_BROWSER_NETWORK_SERVICE_BV_WEB_RESOURCE_OVERRIDE_RESQUEST_H_
#define BISON_BROWSER_NETWORK_SERVICE_BV_WEB_RESOURCE_OVERRIDE_RESQUEST_H_



#include <memory>
#include <string>
#include <vector>

#include "base/android/scoped_java_ref.h"

namespace net {
class HttpRequestHeaders;
}

namespace bison {

struct BvWebResourceRequest;

class BvWebResourceOverrideRequest {
 public:
  BvWebResourceOverrideRequest() = delete;

  // It is expected that |obj| is an instance of the Java-side
  // im.shimo.bison.BvWebResourceOverrideRequest class.
  explicit BvWebResourceOverrideRequest(
      const base::android::JavaRef<jobject>& obj);
  ~BvWebResourceOverrideRequest();

  // True if the call to overrideRequest raised an exception.
  bool RaisedException(JNIEnv* env) const;

  // True if this object contains a request.
  bool HasRequest(JNIEnv* env) const;

  std::string GetRequestUrl(JNIEnv* env) const;
  std::string GetRequestMethod(JNIEnv* env) const;
  void GetRequestHeaders(JNIEnv* env, net::HttpRequestHeaders* headers);

 private:
  base::android::ScopedJavaGlobalRef<jobject> java_object_;


};

}  // namespace bison

#endif  // BISON_BROWSER_NETWORK_SERVICE_BV_WEB_RESOURCE_OVERRIDE_RESQUEST_H_
