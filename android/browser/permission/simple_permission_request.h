// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_BROWSER_PERMISSION_SIMPLE_PERMISSION_REQUEST_H_
#define BISON_ANDROID_BROWSER_PERMISSION_SIMPLE_PERMISSION_REQUEST_H_

#include <stdint.h>

#include "bison/android/browser/permission/bison_permission_request_delegate.h"
#include "base/callback.h"
#include "base/macros.h"

namespace bison {

// The class is used to handle the simple permission request which just needs
// a callback with bool parameter to indicate the permission granted or not.
class SimplePermissionRequest : public BisonPermissionRequestDelegate {
 public:
  SimplePermissionRequest(const GURL& origin,
                          int64_t resources,
                          base::OnceCallback<void(bool)> callback);
  ~SimplePermissionRequest() override;

  // BisonPermissionRequestDelegate implementation.
  const GURL& GetOrigin() override;
  int64_t GetResources() override;
  void NotifyRequestResult(bool allowed) override;

 private:
  const GURL origin_;
  int64_t resources_;
  base::OnceCallback<void(bool)> callback_;

  DISALLOW_COPY_AND_ASSIGN(SimplePermissionRequest);
};

}  // namespace bison

#endif  // BISON_ANDROID_BROWSER_PERMISSION_SIMPLE_PERMISSION_REQUEST_H_
