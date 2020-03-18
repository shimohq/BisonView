// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_BROWSER_PERMISSION_PERMISSION_REQUEST_HANDLER_CLIENT_H_
#define BISON_ANDROID_BROWSER_PERMISSION_PERMISSION_REQUEST_HANDLER_CLIENT_H_

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

#endif  // BISON_ANDROID_BROWSER_PERMISSION_PERMISSION_REQUEST_HANDLER_CLIENT_H_
