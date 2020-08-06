// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_BROWSER_JS_JAVA_INTERACTION_JS_TO_JAVA_MESSAGING_H_
#define BISON_CORE_BROWSER_JS_JAVA_INTERACTION_JS_TO_JAVA_MESSAGING_H_

#include <vector>

#include "bison/core/browser/js_java_interaction/js_reply_proxy.h"
#include "bison/core/common/js_java_interaction/interfaces.mojom.h"
#include "base/android/scoped_java_ref.h"
#include "base/strings/string16.h"
#include "mojo/public/cpp/bindings/associated_receiver_set.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "net/proxy_resolution/proxy_bypass_rules.h"

namespace content {
class RenderFrameHost;
}

namespace bison {

// Implementation of mojo::JsToJavaMessaging interface. Receives PostMessage()
// call from renderer JsBinding.
class JsToJavaMessaging : public mojom::JsToJavaMessaging {
 public:
  JsToJavaMessaging(
      content::RenderFrameHost* rfh,
      mojo::PendingAssociatedReceiver<mojom::JsToJavaMessaging> receiver,
      base::android::ScopedJavaGlobalRef<jobject> listener_ref,
      const net::ProxyBypassRules& allowed_origin_rules);
  ~JsToJavaMessaging() override;

  // mojom::JsToJavaMessaging implementation.
  void PostMessage(const base::string16& message,
                   std::vector<mojo::ScopedMessagePipeHandle> ports) override;
  void SetJavaToJsMessaging(mojo::PendingRemote<mojom::JavaToJsMessaging>
                                java_to_js_messaging) override;

 private:
  content::RenderFrameHost* render_frame_host_;
  std::unique_ptr<JsReplyProxy> reply_proxy_;
  base::android::ScopedJavaGlobalRef<jobject> listener_ref_;
  net::ProxyBypassRules allowed_origin_rules_;
  mojo::AssociatedReceiver<mojom::JsToJavaMessaging> receiver_{this};

  DISALLOW_COPY_AND_ASSIGN(JsToJavaMessaging);
};

}  // namespace bison

#endif  // BISON_CORE_BROWSER_JS_JAVA_INTERACTION_JS_TO_JAVA_MESSAGING_H_