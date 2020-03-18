// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/browser/bison_variations_seed_bridge.h"

#include <jni.h>
#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "bison/android/browser_jni_headers/BisonVariationsSeedBridge_jni.h"
#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "components/variations/seed_response.h"

namespace bison {

std::unique_ptr<variations::SeedResponse> GetAndClearJavaSeed() {
  JNIEnv* env = base::android::AttachCurrentThread();
  if (!Java_BisonVariationsSeedBridge_haveSeed(env))
    return nullptr;

  base::android::ScopedJavaLocalRef<jstring> j_signature =
      Java_BisonVariationsSeedBridge_getSignature(env);
  base::android::ScopedJavaLocalRef<jstring> j_country =
      Java_BisonVariationsSeedBridge_getCountry(env);
  jlong j_date = Java_BisonVariationsSeedBridge_getDate(env);
  jboolean j_is_gzip_compressed =
      Java_BisonVariationsSeedBridge_getIsGzipCompressed(env);
  base::android::ScopedJavaLocalRef<jbyteArray> j_data =
      Java_BisonVariationsSeedBridge_getData(env);
  Java_BisonVariationsSeedBridge_clearSeed(env);

  auto java_seed = std::make_unique<variations::SeedResponse>();
  base::android::JavaByteArrayToString(env, j_data, &java_seed->data);
  java_seed->signature = base::android::ConvertJavaStringToUTF8(j_signature);
  java_seed->country = base::android::ConvertJavaStringToUTF8(j_country);
  java_seed->date = j_date;
  java_seed->is_gzip_compressed = static_cast<bool>(j_is_gzip_compressed);
  return java_seed;
}

bool IsSeedFresh() {
  JNIEnv* env = base::android::AttachCurrentThread();
  return static_cast<bool>(Java_BisonVariationsSeedBridge_isLoadedSeedFresh(env));
}

}  // namespace bison
