// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_BROWSER_TRACING_BISON_TRACING_CONTROLLER_H_
#define BISON_ANDROID_BROWSER_TRACING_BISON_TRACING_CONTROLLER_H_

#include "base/android/jni_weak_ref.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace bison {

class BisonTracingController {
 public:
  BisonTracingController(JNIEnv* env, jobject obj);

  bool Start(JNIEnv* env,
             const base::android::JavaParamRef<jobject>& obj,
             const base::android::JavaParamRef<jstring>& categories,
             jint mode);
  bool StopAndFlush(JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& obj);
  bool IsTracing(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

 private:
  ~BisonTracingController();

  void OnTraceDataReceived(std::unique_ptr<std::string> chunk);
  void OnTraceDataComplete();

  JavaObjectWeakGlobalRef weak_java_object_;
  base::WeakPtrFactory<BisonTracingController> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(BisonTracingController);
};

}  // namespace bison

#endif  // BISON_ANDROID_BROWSER_TRACING_BISON_TRACING_CONTROLLER_H_
