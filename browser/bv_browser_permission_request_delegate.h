// create by jiang947

#ifndef BISON_BROWSER_BISON_BROWSER_PERMISSION_REQUEST_DELEGATE_H_
#define BISON_BROWSER_BISON_BROWSER_PERMISSION_REQUEST_DELEGATE_H_

#include "base/callback_forward.h"
#include "bison/browser/permission/permission_callback.h"
#include "url/gurl.h"

namespace bison {

// Delegate interface to handle the permission requests from |BrowserContext|.
class BvBrowserPermissionRequestDelegate {
 public:
  // Returns the BvBrowserPermissionRequestDelegate instance associated with
  // the given render_process_id and render_frame_id, or NULL.
  static BvBrowserPermissionRequestDelegate* FromID(int render_process_id,
                                                    int render_frame_id);

  virtual void RequestProtectedMediaIdentifierPermission(
      const GURL& origin,
      PermissionCallback callback) = 0;

  virtual void CancelProtectedMediaIdentifierPermissionRequests(
      const GURL& origin) = 0;

  virtual void RequestGeolocationPermission(const GURL& origin,
                                            PermissionCallback callback) = 0;

  virtual void CancelGeolocationPermissionRequests(const GURL& origin) = 0;

  virtual void RequestMIDISysexPermission(const GURL& origin,
                                          PermissionCallback callback) = 0;

  virtual void CancelMIDISysexPermissionRequests(const GURL& origin) = 0;

 protected:
  BvBrowserPermissionRequestDelegate() {}
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_BROWSER_PERMISSION_REQUEST_DELEGATE_H_
