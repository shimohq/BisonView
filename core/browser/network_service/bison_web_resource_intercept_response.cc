// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/core/browser/network_service/bison_web_resource_intercept_response.h"

#include <memory>
#include <utility>

#include "bison/core/browser/network_service/bison_web_resource_response.h"
#include "bison/core/browser_jni_headers/BisonWebResourceInterceptResponse_jni.h"
#include "base/android/jni_android.h"

using base::android::ScopedJavaLocalRef;

namespace bison {

BisonWebResourceInterceptResponse::BisonWebResourceInterceptResponse(
    const base::android::JavaRef<jobject>& obj)
    : java_object_(obj) {}

BisonWebResourceInterceptResponse::~BisonWebResourceInterceptResponse() = default;

bool BisonWebResourceInterceptResponse::RaisedException(JNIEnv* env) const {
  return Java_BisonWebResourceInterceptResponse_getRaisedException(env,
                                                                java_object_);
}

bool BisonWebResourceInterceptResponse::HasResponse(JNIEnv* env) const {
  return !Java_BisonWebResourceInterceptResponse_getResponse(env, java_object_)
              .is_null();
}

std::unique_ptr<BisonWebResourceResponse>
BisonWebResourceInterceptResponse::GetResponse(JNIEnv* env) const {
  ScopedJavaLocalRef<jobject> j_response =
      Java_BisonWebResourceInterceptResponse_getResponse(env, java_object_);
  if (j_response.is_null())
    return nullptr;
  return std::make_unique<BisonWebResourceResponse>(j_response);
}

}  // namespace bison
