// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_BROWSER_BISON_BROWSER_PERMISSION_REQUEST_DELEGATE_H_
#define BISON_CORE_BROWSER_BISON_BROWSER_PERMISSION_REQUEST_DELEGATE_H_

#include "base/callback_forward.h"
#include "url/gurl.h"

namespace bison {

// Delegate interface to handle the permission requests from |BrowserContext|.
class BisonBrowserPermissionRequestDelegate {
 public:
  // Returns the BisonBrowserPermissionRequestDelegate instance associated with
  // the given render_process_id and render_frame_id, or NULL.
  static BisonBrowserPermissionRequestDelegate* FromID(int render_process_id,
                                                    int render_frame_id);

  virtual void RequestProtectedMediaIdentifierPermission(
      const GURL& origin,
      base::OnceCallback<void(bool)> callback) = 0;

  virtual void CancelProtectedMediaIdentifierPermissionRequests(
      const GURL& origin) = 0;

  virtual void RequestGeolocationPermission(
      const GURL& origin,
      base::OnceCallback<void(bool)> callback) = 0;

  virtual void CancelGeolocationPermissionRequests(const GURL& origin) = 0;

  virtual void RequestMIDISysexPermission(
      const GURL& origin,
      base::OnceCallback<void(bool)> callback) = 0;

  virtual void CancelMIDISysexPermissionRequests(const GURL& origin) = 0;

 protected:
  BisonBrowserPermissionRequestDelegate() {}
};

}  // namespace bison
#endif  // BISON_CORE_BROWSER_BISON_BROWSER_PERMISSION_REQUEST_DELEGATE_H_
