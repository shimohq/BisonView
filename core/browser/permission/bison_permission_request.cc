// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/core/browser/permission/bison_permission_request.h"

#include <utility>

#include "bison/core/browser/permission/bison_permission_request_delegate.h"
#include "bison/core/browser_jni_headers/BisonPermissionRequest_jni.h"
#include "base/android/jni_string.h"

using base::android::AttachCurrentThread;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace bison {

// static
base::android::ScopedJavaLocalRef<jobject> BisonPermissionRequest::Create(
    std::unique_ptr<BisonPermissionRequestDelegate> delegate,
    base::WeakPtr<BisonPermissionRequest>* weak_ptr) {
  base::android::ScopedJavaLocalRef<jobject> java_peer;
  BisonPermissionRequest* permission_request =
      new BisonPermissionRequest(std::move(delegate), &java_peer);
  *weak_ptr = permission_request->weak_factory_.GetWeakPtr();
  return java_peer;
}

BisonPermissionRequest::BisonPermissionRequest(
    std::unique_ptr<BisonPermissionRequestDelegate> delegate,
    ScopedJavaLocalRef<jobject>* java_peer)
    : delegate_(std::move(delegate)), processed_(false) {
  DCHECK(delegate_.get());
  DCHECK(java_peer);

  JNIEnv* env = AttachCurrentThread();
  *java_peer = Java_BisonPermissionRequest_create(
      env, reinterpret_cast<jlong>(this),
      ConvertUTF8ToJavaString(env, GetOrigin().spec()), GetResources());
  java_ref_ = JavaObjectWeakGlobalRef(env, java_peer->obj());
}

BisonPermissionRequest::~BisonPermissionRequest() {
  OnAcceptInternal(false);
}

void BisonPermissionRequest::OnAccept(JNIEnv* env,
                                   const JavaParamRef<jobject>& jcaller,
                                   jboolean accept) {
  OnAcceptInternal(accept);
}

void BisonPermissionRequest::OnAcceptInternal(bool accept) {
  if (!processed_) {
    delegate_->NotifyRequestResult(accept);
    processed_ = true;
  }
}

void BisonPermissionRequest::DeleteThis() {
  ScopedJavaLocalRef<jobject> j_request = GetJavaObject();
  if (j_request.is_null())
    return;
  Java_BisonPermissionRequest_destroyNative(AttachCurrentThread(), j_request);
}

void BisonPermissionRequest::Destroy(JNIEnv* env) {
  delete this;
}

ScopedJavaLocalRef<jobject> BisonPermissionRequest::GetJavaObject() {
  return java_ref_.get(AttachCurrentThread());
}

const GURL& BisonPermissionRequest::GetOrigin() {
  return delegate_->GetOrigin();
}

int64_t BisonPermissionRequest::GetResources() {
  return delegate_->GetResources();
}

void BisonPermissionRequest::CancelAndDelete() {
  processed_ = true;
  DeleteThis();
}

}  // namespace bison
