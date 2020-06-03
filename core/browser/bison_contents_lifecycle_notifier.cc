// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/core/browser/bison_contents_lifecycle_notifier.h"

#include "bison/core/browser_jni_headers/BisonContentsLifecycleNotifier_jni.h"

using base::android::AttachCurrentThread;

namespace bison {

// static
void BisonContentsLifecycleNotifier::OnWebViewCreated() {
  Java_BisonContentsLifecycleNotifier_onWebViewCreated(AttachCurrentThread());
}

// static
void BisonContentsLifecycleNotifier::OnWebViewDestroyed() {
  Java_BisonContentsLifecycleNotifier_onWebViewDestroyed(AttachCurrentThread());
}

}  // namespace bison
