// create by jiang947 


#ifndef BISON_BROWSER_NETWORK_SERVICE_BISON_WEB_RESOURCE_OVERRITE_RESQUEST_H_
#define BISON_BROWSER_NETWORK_SERVICE_BISON_WEB_RESOURCE_OVERRITE_RESQUEST_H_

#include <memory>
#include <vector>
#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"


namespace net {
class HttpRequestHeaders;
}

namespace bison {

struct BisonWebResourceRequest;

class BisonWebResourceOverriteRequest {
 public:
  BisonWebResourceOverriteRequest() = delete;

  // It is expected that |obj| is an instance of the Java-side
  // im.shimo.bison.BisonWebResourceOverriteRequest class.
  explicit BisonWebResourceOverriteRequest(
      const base::android::JavaRef<jobject>& obj);
  ~BisonWebResourceOverriteRequest();

    // True if the call to shouldOverriteRequest raised an exception.
  bool RaisedException(JNIEnv* env) const;

  // True if this object contains a request.
  bool HasRequest(JNIEnv* env) const;

  std::string GetRequestUrl(JNIEnv* env) const;
  std::string GetRequestMethod(JNIEnv* env) const;
  void GetRequestHeaders(JNIEnv* env, net::HttpRequestHeaders* headers);



 private:
  base::android::ScopedJavaGlobalRef<jobject> java_object_;

  DISALLOW_COPY_AND_ASSIGN(BisonWebResourceOverriteRequest);
};

}  // namespace bison

#endif  // BISON_BROWSER_NETWORK_SERVICE_BISON_WEB_RESOURCE_OVERRITE_RESQUEST_H_
