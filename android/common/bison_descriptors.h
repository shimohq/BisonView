// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_COMMON_BISON_DESCRIPTORS_H_
#define BISON_ANDROID_COMMON_BISON_DESCRIPTORS_H_

#include "content/public/common/content_descriptors.h"

enum {
  kAndroidWebViewLocalePakDescriptor = kContentIPCDescriptorMax + 1,
  kAndroidWebViewMainPakDescriptor,
  kAndroidWebView100PercentPakDescriptor,
  kAndroidWebViewCrashSignalDescriptor,
  kAndroidMinidumpDescriptor,
};

#endif  // BISON_ANDROID_COMMON_BISON_DESCRIPTORS_H_
