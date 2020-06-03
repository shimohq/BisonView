// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_BROWSER_BISON_QUOTA_PERMISSION_CONTEXT_H_
#define BISON_CORE_BROWSER_BISON_QUOTA_PERMISSION_CONTEXT_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/quota_permission_context.h"

namespace bison {

class BisonQuotaPermissionContext : public content::QuotaPermissionContext {
 public:
  BisonQuotaPermissionContext();

  void RequestQuotaPermission(const content::StorageQuotaParams& params,
                              int render_process_id,
                              const PermissionCallback& callback) override;

 private:
  ~BisonQuotaPermissionContext() override;

  DISALLOW_COPY_AND_ASSIGN(BisonQuotaPermissionContext);
};

}  // namespace bison

#endif  // BISON_CORE_BROWSER_BISON_QUOTA_PERMISSION_CONTEXT_H_

