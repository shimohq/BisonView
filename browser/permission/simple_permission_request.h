// create by jiang947

#ifndef BISON_BROWSER_PERMISSION_SIMPLE_PERMISSION_REQUEST_H_
#define BISON_BROWSER_PERMISSION_SIMPLE_PERMISSION_REQUEST_H_

#include <stdint.h>

#include "base/callback.h"
#include "bison/browser/permission/bv_permission_request_delegate.h"
#include "bison/browser/permission/permission_callback.h"

namespace bison {

// The class is used to handle the simple permission request which just needs
// a callback with bool parameter to indicate the permission granted or not.
class SimplePermissionRequest : public BvPermissionRequestDelegate {
 public:
  SimplePermissionRequest(const GURL& origin,
                          int64_t resources,
                          base::OnceCallback<void(bool)> callback);
  SimplePermissionRequest(const SimplePermissionRequest&) = delete;
  SimplePermissionRequest& operator=(const SimplePermissionRequest&) = delete;

  ~SimplePermissionRequest() override;

  // BvPermissionRequestDelegate implementation.
  const GURL& GetOrigin() override;
  int64_t GetResources() override;
  void NotifyRequestResult(bool allowed) override;

 private:
  const GURL origin_;
  int64_t resources_;
  PermissionCallback callback_;


};

}  // namespace bison

#endif  // BISON_BROWSER_PERMISSION_SIMPLE_PERMISSION_REQUEST_H_
