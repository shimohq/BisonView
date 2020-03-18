// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/renderer/bison_websocket_handshake_throttle_provider.h"

#include <utility>

#include "base/feature_list.h"
#include "components/safe_browsing/features.h"
#include "components/safe_browsing/renderer/websocket_sb_handshake_throttle.h"
#include "content/public/common/content_features.h"
#include "content/public/renderer/render_thread.h"
#include "third_party/blink/public/platform/websocket_handshake_throttle.h"

namespace bison {

BisonWebSocketHandshakeThrottleProvider::BisonWebSocketHandshakeThrottleProvider(
    blink::ThreadSafeBrowserInterfaceBrokerProxy* broker) {
  DETACH_FROM_THREAD(thread_checker_);
  broker->GetInterface(safe_browsing_remote_.InitWithNewPipeAndPassReceiver());
}

BisonWebSocketHandshakeThrottleProvider::~BisonWebSocketHandshakeThrottleProvider() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

BisonWebSocketHandshakeThrottleProvider::BisonWebSocketHandshakeThrottleProvider(
    const BisonWebSocketHandshakeThrottleProvider& other) {
  DETACH_FROM_THREAD(thread_checker_);
  if (other.safe_browsing_) {
    other.safe_browsing_->Clone(
        safe_browsing_remote_.InitWithNewPipeAndPassReceiver());
  }
}

std::unique_ptr<content::WebSocketHandshakeThrottleProvider>
BisonWebSocketHandshakeThrottleProvider::Clone(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (safe_browsing_remote_) {
    safe_browsing_.Bind(std::move(safe_browsing_remote_),
                        std::move(task_runner));
  }
  return base::WrapUnique(new BisonWebSocketHandshakeThrottleProvider(*this));
}

std::unique_ptr<blink::WebSocketHandshakeThrottle>
BisonWebSocketHandshakeThrottleProvider::CreateThrottle(
    int render_frame_id,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (safe_browsing_remote_) {
    safe_browsing_.Bind(std::move(safe_browsing_remote_),
                        std::move(task_runner));
  }
  return std::make_unique<safe_browsing::WebSocketSBHandshakeThrottle>(
      safe_browsing_.get(), render_frame_id);
}

}  // namespace bison
