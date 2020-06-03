// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/core/browser/permission/simple_permission_request.h"

#include "bison/core/browser/permission/bison_permission_request.h"
#include "base/callback.h"

namespace bison {

SimplePermissionRequest::SimplePermissionRequest(
    const GURL& origin,
    int64_t resources,
    base::OnceCallback<void(bool)> callback)
    : origin_(origin), resources_(resources), callback_(std::move(callback)) {}

SimplePermissionRequest::~SimplePermissionRequest() {}

void SimplePermissionRequest::NotifyRequestResult(bool allowed) {
  std::move(callback_).Run(allowed);
}

const GURL& SimplePermissionRequest::GetOrigin() {
  return origin_;
}

int64_t SimplePermissionRequest::GetResources() {
  return resources_;
}

}  // namespace bison
