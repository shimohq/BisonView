// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison_resource.h"

#include "bison/core/common_jni_headers/BisonResource_jni.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"

using base::android::ScopedJavaLocalRef;

namespace bison {
namespace BisonResource {


// TODO: jiang 这里要屏蔽
std::vector<std::string> GetConfigKeySystemUuidMapping() {
  JNIEnv* env = base::android::AttachCurrentThread();
  std::vector<std::string> key_system_uuid_mappings;
  ScopedJavaLocalRef<jobjectArray> mappings =
      Java_BisonResource_getConfigKeySystemUuidMapping(env);
  base::android::AppendJavaStringArrayToStringVector(env, mappings,
                                                     &key_system_uuid_mappings);
  return key_system_uuid_mappings;
}

}  // namespace BisonResource
}  // namespace bison
