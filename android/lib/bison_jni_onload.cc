// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/lib/bison_jni_onload.h"

#include "bison/android/lib/bison_main_delegate.h"
#include "base/android/library_loader/library_loader_hooks.h"
#include "components/version_info/version_info_values.h"
#include "content/public/app/content_jni_onload.h"
#include "content/public/app/content_main.h"
#include "bison/android/lib/log_util.h"

namespace bison {

bool OnJNIOnLoadInit() {
  LOGE("bison jni on load -> OnJNIOnLoadInit");
  if (!content::android::OnJNIOnLoadInit()){
    LOGE("content::android::OnJNIOnLoadInit() failure");
    return false;
  }
  LOGE("content::android::OnJNIOnLoadInit() success");


  base::android::SetVersionNumber(PRODUCT_VERSION);
  content::SetContentMainDelegate(new bison::BisonMainDelegate());
  return true;
}

}  // namespace bison
