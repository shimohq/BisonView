// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/browser/bison_quota_permission_context.h"

#include "base/logging.h"

using content::QuotaPermissionContext;

namespace bison {

BisonQuotaPermissionContext::BisonQuotaPermissionContext() {
}

BisonQuotaPermissionContext::~BisonQuotaPermissionContext() {
}

void BisonQuotaPermissionContext::RequestQuotaPermission(
    const content::StorageQuotaParams& params,
    int render_process_id,
    const PermissionCallback& callback) {
  // Android WebView only uses storage::kStorageTypeTemporary type of storage
  // with quota managed automatically, not through this interface. Therefore
  // unconditionally disallow all quota requests here.
  callback.Run(QUOTA_PERMISSION_RESPONSE_DISALLOW);
}

}  // namespace bison
