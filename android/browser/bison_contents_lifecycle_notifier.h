// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_BROWSER_BISON_CONTENTS_LIFECYCLE_NOTIFIER_H_
#define BISON_ANDROID_BROWSER_BISON_CONTENTS_LIFECYCLE_NOTIFIER_H_

#include "base/android/jni_android.h"
#include "base/macros.h"

namespace bison {

class BisonContentsLifecycleNotifier {
 public:
  static void OnWebViewCreated();
  static void OnWebViewDestroyed();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BisonContentsLifecycleNotifier);
};

}  // namespace bison

#endif  // BISON_ANDROID_BROWSER_BISON_CONTENTS_LIFECYCLE_NOTIFIER_H_
