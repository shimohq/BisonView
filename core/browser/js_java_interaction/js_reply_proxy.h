// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_BROWSER_JS_JAVA_INTERACTION_JS_REPLY_PROXY_H_
#define BISON_CORE_BROWSER_JS_JAVA_INTERACTION_JS_REPLY_PROXY_H_
          
#include "bison/core/common/js_java_interaction/interfaces.mojom.h"
#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace bison {

class JsReplyProxy {
 public:
  explicit JsReplyProxy(
      mojo::PendingRemote<mojom::JavaToJsMessaging> java_to_js_messaging);

  ~JsReplyProxy();

  base::android::ScopedJavaLocalRef<jobject> GetJavaPeer();

  void PostMessage(JNIEnv* env,
                   const base::android::JavaParamRef<jstring>& message);

 private:
  JavaObjectWeakGlobalRef java_ref_;
  mojo::Remote<mojom::JavaToJsMessaging> java_to_js_messaging_;

  DISALLOW_COPY_AND_ASSIGN(JsReplyProxy);
};

}  // namespace bison

#endif  // BISON_CORE_BROWSER_JS_JAVA_INTERACTION_JS_REPLY_PROXY_H_
