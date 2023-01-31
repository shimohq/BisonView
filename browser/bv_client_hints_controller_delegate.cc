// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/browser/bv_client_hints_controller_delegate.h"

#include "base/notreached.h"
#include "content/public/browser/client_hints_controller_delegate.h"
#include "content/public/browser/render_frame_host.h"
#include "services/network/public/cpp/network_quality_tracker.h"
#include "services/network/public/mojom/web_client_hints_types.mojom.h"
#include "third_party/blink/public/common/client_hints/enabled_client_hints.h"
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"
#include "ui/gfx/geometry/size_f.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace bison {

BvClientHintsControllerDelegate::BvClientHintsControllerDelegate() {
  // TODO(crbug.com/921655): Actually implement function.
  NOTIMPLEMENTED();
}
BvClientHintsControllerDelegate::~BvClientHintsControllerDelegate() {
  // TODO(crbug.com/921655): Actually implement function.
  NOTIMPLEMENTED();
}

network::NetworkQualityTracker*
BvClientHintsControllerDelegate::GetNetworkQualityTracker() {
  // TODO(crbug.com/921655): Actually implement function.
  NOTIMPLEMENTED();
  return nullptr;
}

void BvClientHintsControllerDelegate::GetAllowedClientHintsFromSource(
    const url::Origin& origin,
    blink::EnabledClientHints* client_hints) {
  // TODO(crbug.com/921655): Actually implement function.
  NOTIMPLEMENTED();
}

bool BvClientHintsControllerDelegate::IsJavaScriptAllowed(
    const GURL& url,
    content::RenderFrameHost* parent_rfh) {
  // TODO(crbug.com/921655): Actually implement function.
  NOTIMPLEMENTED();
  return false;
}

bool BvClientHintsControllerDelegate::AreThirdPartyCookiesBlocked(
    const GURL& url) {
  // TODO(crbug.com/921655): Actually implement function.
  NOTIMPLEMENTED();
  return false;
}

blink::UserAgentMetadata
BvClientHintsControllerDelegate::GetUserAgentMetadata() {
  // TODO(crbug.com/921655): Actually implement function.
  NOTIMPLEMENTED();
  return blink::UserAgentMetadata();
}

void BvClientHintsControllerDelegate::PersistClientHints(
    const url::Origin& primary_origin,
    content::RenderFrameHost* parent_rfh,
    const std::vector<network::mojom::WebClientHintsType>& client_hints) {
  // TODO(crbug.com/921655): Actually implement function.
  NOTIMPLEMENTED();
}

void BvClientHintsControllerDelegate::SetAdditionalClientHints(
    const std::vector<network::mojom::WebClientHintsType>&) {
  // TODO(crbug.com/921655): Actually implement function.
  NOTIMPLEMENTED();
}

void BvClientHintsControllerDelegate::ClearAdditionalClientHints() {
  // TODO(crbug.com/921655): Actually implement function.
  NOTIMPLEMENTED();
}

void BvClientHintsControllerDelegate::SetMostRecentMainFrameViewportSize(
    const gfx::Size& viewport_size) {
  // TODO(crbug.com/921655): Actually implement function.
  NOTIMPLEMENTED();
}

gfx::Size
BvClientHintsControllerDelegate::GetMostRecentMainFrameViewportSize() {
  // TODO(crbug.com/921655): Actually implement function.
  NOTIMPLEMENTED();
  return gfx::Size(0, 0);
}

}  // namespace bison
