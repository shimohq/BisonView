// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/bison_jni_registration.h"

#include "base/android/jni_android.h"
#include "base/android/library_loader/library_loader_hooks.h"
#include "base/android/jni_utils.h"

#include "bison/android/lib/bison_jni_onload.h"
#include "bison/android/lib/log_util.h"

#if defined(WEBVIEW_INCLUDES_WEBLAYER)
#include "weblayer/app/jni_onload.h"
#endif

namespace {

bool NativeInit(base::android::LibraryProcessType library_process_type) {
  LOGE("NativeInit :library_process_type is %d",library_process_type);
  switch (library_process_type) {
#if defined(WEBVIEW_INCLUDES_WEBLAYER)
    case base::android::PROCESS_WEBLAYER:
    case base::android::PROCESS_WEBLAYER_CHILD:
      return weblayer::OnJNIOnLoadInit();
      break;
#endif

    default:
      LOGE("NativeInit : run OnJNIOnLoadInit");
      return bison::OnJNIOnLoadInit();
  }
}

}  // namespace


JNI_EXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  base::android::InitVM(vm);
  JNIEnv* env = base::android::AttachCurrentThread();
  if (!base::android::IsSelectiveJniRegistrationEnabled(env)) {
    if (!RegisterNonMainDexNatives(env)) {
      return false;
    }
  }

  if (!RegisterMainDexNatives(env)) {
    return false;
  }
  LOGD("bison loaded!");

  base::android::SetNativeInitializationHook(&NativeInit);
  return JNI_VERSION_1_4;
}
