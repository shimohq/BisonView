// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/common/bv_media_drm_bridge_client.h"

#include <utility>

#include "base/logging.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace bison {

BvMediaDrmBridgeClient::BvMediaDrmBridgeClient() {}

BvMediaDrmBridgeClient::~BvMediaDrmBridgeClient() {}

media::MediaDrmBridgeDelegate*
BvMediaDrmBridgeClient::GetMediaDrmBridgeDelegate(
    const media::UUID& scheme_uuid) {
  if (scheme_uuid == widevine_delegate_.GetUUID())
    return &widevine_delegate_;
  return nullptr;
}

}  // bison
