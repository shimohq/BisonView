// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_COMMON_BV_MEDIA_DRM_BRIDGE_CLIENT_H_
#define BISON_COMMON_BV_MEDIA_DRM_BRIDGE_CLIENT_H_

#include "components/cdm/common/widevine_drm_delegate_android.h"
#include "media/base/android/media_drm_bridge_client.h"

namespace bison {

class BvMediaDrmBridgeClient : public media::MediaDrmBridgeClient {
 public:
  explicit BvMediaDrmBridgeClient();
  BvMediaDrmBridgeClient(const BvMediaDrmBridgeClient&) = delete;
  BvMediaDrmBridgeClient& operator=(const BvMediaDrmBridgeClient&) = delete;

  ~BvMediaDrmBridgeClient() override;

 private:
  // media::MediaDrmBridgeClient implementation:
  media::MediaDrmBridgeDelegate* GetMediaDrmBridgeDelegate(
      const media::UUID& scheme_uuid) override;

  cdm::WidevineDrmDelegateAndroid widevine_delegate_;
};

}  // namespace bison

#endif  // BISON_COMMON_BV_MEDIA_DRM_BRIDGE_CLIENT_H_
