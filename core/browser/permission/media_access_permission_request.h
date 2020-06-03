// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_BROWSER_PERMISSION_MEDIA_ACCESS_PERMISSION_REQUEST_H_
#define BISON_CORE_BROWSER_PERMISSION_MEDIA_ACCESS_PERMISSION_REQUEST_H_

#include <stdint.h>

#include "bison/core/browser/permission/bison_permission_request_delegate.h"
#include "base/callback.h"
#include "base/macros.h"
#include "content/public/browser/media_stream_request.h"
#include "third_party/blink/public/common/mediastream/media_stream_request.h"

namespace bison {

// The BisonPermissionRequestDelegate implementation for media access permission
// request.
class MediaAccessPermissionRequest : public BisonPermissionRequestDelegate {
 public:
  MediaAccessPermissionRequest(const content::MediaStreamRequest& request,
                               content::MediaResponseCallback callback);
  ~MediaAccessPermissionRequest() override;

  // BisonPermissionRequestDelegate implementation.
  const GURL& GetOrigin() override;
  int64_t GetResources() override;
  void NotifyRequestResult(bool allowed) override;

 private:
  friend class TestMediaAccessPermissionRequest;

  const content::MediaStreamRequest request_;
  content::MediaResponseCallback callback_;

  // For test only.
  blink::MediaStreamDevices audio_test_devices_;
  blink::MediaStreamDevices video_test_devices_;

  DISALLOW_COPY_AND_ASSIGN(MediaAccessPermissionRequest);
};

}  // namespace bison

#endif  // BISON_CORE_BROWSER_PERMISSION_MEDIA_ACCESS_PERMISSION_REQUEST_H_
