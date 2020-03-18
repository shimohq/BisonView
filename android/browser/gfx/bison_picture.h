// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_BROWSER_GFX_BISON_PICTURE_H_
#define BISON_ANDROID_BROWSER_GFX_BISON_PICTURE_H_

#include <memory>

#include "base/android/jni_weak_ref.h"
#include "base/macros.h"
#include "third_party/skia/include/core/SkRefCnt.h"

class SkPicture;

namespace bison {

class BisonPicture {
 public:
  BisonPicture(sk_sp<SkPicture> picture);
  ~BisonPicture();

  // Methods called from Java.
  void Destroy(JNIEnv* env);
  jint GetWidth(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  jint GetHeight(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void Draw(JNIEnv* env,
            const base::android::JavaParamRef<jobject>& obj,
            const base::android::JavaParamRef<jobject>& canvas);

 private:
  sk_sp<SkPicture> picture_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(BisonPicture);
};

}  // bison

#endif  // BISON_ANDROID_BROWSER_GFX_BISON_PICTURE_H_
