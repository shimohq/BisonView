// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/browser/bison_quota_permission_context.h"


using content::QuotaPermissionContext;

namespace bison {

BisonQuotaPermissionContext::BisonQuotaPermissionContext() {
}

BisonQuotaPermissionContext::~BisonQuotaPermissionContext() {
}

void BisonQuotaPermissionContext::RequestQuotaPermission(
    const content::StorageQuotaParams& params,
    int render_process_id,
    PermissionCallback callback) {
  // BisonView only uses storage::kStorageTypeTemporary type of storage
  // with quota managed automatically, not through this interface. Therefore
  // unconditionally disallow all quota requests here.
  std::move(callback).Run(QUOTA_PERMISSION_RESPONSE_DISALLOW);
}

}  // namespace bison
