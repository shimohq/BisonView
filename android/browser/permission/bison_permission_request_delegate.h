// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_BROWSER_PERMISSION_BISON_PERMISSION_REQUEST_DELEGATE_H_
#define BISON_ANDROID_BROWSER_PERMISSION_BISON_PERMISSION_REQUEST_DELEGATE_H_

#include <stdint.h>

#include "base/macros.h"
#include "url/gurl.h"

namespace bison {

// The delegate interface to be implemented for a specific permission request.
class BisonPermissionRequestDelegate {
 public:
  BisonPermissionRequestDelegate();
  virtual ~BisonPermissionRequestDelegate();

  // Get the origin which initiated the permission request.
  virtual const GURL& GetOrigin() = 0;

  // Get the resources the origin wanted to access.
  virtual int64_t GetResources() = 0;

  // Notify the permission request is allowed or not.
  virtual void NotifyRequestResult(bool allowed) = 0;

  DISALLOW_COPY_AND_ASSIGN(BisonPermissionRequestDelegate);
};

}  // namespace bison

#endif  // BISON_ANDROID_BROWSER_PERMISSION_BISON_PERMISSION_REQUEST_DELEGATE_H_
