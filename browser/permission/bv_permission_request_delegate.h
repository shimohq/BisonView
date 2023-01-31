// create by jiang947

#ifndef BISON_BROWSER_PERMISSION_BISON_PERMISSION_REQUEST_DELEGATE_H_
#define BISON_BROWSER_PERMISSION_BISON_PERMISSION_REQUEST_DELEGATE_H_

#include <stdint.h>

#include "url/gurl.h"

namespace bison {

// The delegate interface to be implemented for a specific permission request.
class BvPermissionRequestDelegate {
 public:
  BvPermissionRequestDelegate();

  BvPermissionRequestDelegate(const BvPermissionRequestDelegate&) = delete;
  BvPermissionRequestDelegate& operator=(const BvPermissionRequestDelegate&) =
      delete;

  virtual ~BvPermissionRequestDelegate();

  // Get the origin which initiated the permission request.
  virtual const GURL& GetOrigin() = 0;

  // Get the resources the origin wanted to access.
  virtual int64_t GetResources() = 0;

  // Notify the permission request is allowed or not.
  virtual void NotifyRequestResult(bool allowed) = 0;
};

}  // namespace bison

#endif  // BISON_BROWSER_PERMISSION_BISON_PERMISSION_REQUEST_DELEGATE_H_
