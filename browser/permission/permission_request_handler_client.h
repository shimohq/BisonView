// create by jiang947 


#ifndef BISON_BROWSER_PERMISSION_PERMISSION_REQUEST_HANDLER_CLIENT_H_
#define BISON_BROWSER_PERMISSION_PERMISSION_REQUEST_HANDLER_CLIENT_H_


#include "base/android/scoped_java_ref.h"

namespace bison {

class BisonPermissionRequest;

class PermissionRequestHandlerClient {
 public:
  PermissionRequestHandlerClient();
  virtual ~PermissionRequestHandlerClient();

  virtual void OnPermissionRequest(
      base::android::ScopedJavaLocalRef<jobject> java_request,
      BisonPermissionRequest* request) = 0;
  virtual void OnPermissionRequestCanceled(BisonPermissionRequest* request) = 0;
};

}  // namespace bison

#endif  // BISON_BROWSER_PERMISSION_PERMISSION_REQUEST_HANDLER_CLIENT_H_
