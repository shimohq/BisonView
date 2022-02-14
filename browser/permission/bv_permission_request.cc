#include "bison/browser/permission/bv_permission_request.h"

#include <utility>

#include "bison/bison_jni_headers/BvPermissionRequest_jni.h"
#include "bison/browser/permission/bv_permission_request_delegate.h"

#include "base/android/jni_string.h"

using base::android::AttachCurrentThread;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace bison {

// static
base::android::ScopedJavaLocalRef<jobject> BvPermissionRequest::Create(
    std::unique_ptr<BvPermissionRequestDelegate> delegate,
    base::WeakPtr<BvPermissionRequest>* weak_ptr) {
  base::android::ScopedJavaLocalRef<jobject> java_peer;
  BvPermissionRequest* permission_request =
      new BvPermissionRequest(std::move(delegate), &java_peer);
  *weak_ptr = permission_request->weak_factory_.GetWeakPtr();
  return java_peer;
}

BvPermissionRequest::BvPermissionRequest(
    std::unique_ptr<BvPermissionRequestDelegate> delegate,
    ScopedJavaLocalRef<jobject>* java_peer)
    : delegate_(std::move(delegate)), processed_(false) {
  DCHECK(delegate_.get());
  DCHECK(java_peer);

  JNIEnv* env = AttachCurrentThread();
  *java_peer = Java_BvPermissionRequest_create(
      env, reinterpret_cast<jlong>(this),
      ConvertUTF8ToJavaString(env, GetOrigin().spec()), GetResources());
  java_ref_ = JavaObjectWeakGlobalRef(env, java_peer->obj());
}

BvPermissionRequest::~BvPermissionRequest() {
  OnAcceptInternal(false);
}

void BvPermissionRequest::OnAccept(JNIEnv* env,
                                   const JavaParamRef<jobject>& jcaller,
                                   jboolean accept) {
  OnAcceptInternal(accept);
}

void BvPermissionRequest::OnAcceptInternal(bool accept) {
  if (!processed_) {
    delegate_->NotifyRequestResult(accept);
    processed_ = true;
  }
}

void BvPermissionRequest::DeleteThis() {
  ScopedJavaLocalRef<jobject> j_request = GetJavaObject();
  if (j_request.is_null())
    return;
  Java_BvPermissionRequest_destroyNative(AttachCurrentThread(), j_request);
}

void BvPermissionRequest::Destroy(JNIEnv* env) {
  delete this;
}

ScopedJavaLocalRef<jobject> BvPermissionRequest::GetJavaObject() {
  return java_ref_.get(AttachCurrentThread());
}

const GURL& BvPermissionRequest::GetOrigin() {
  return delegate_->GetOrigin();
}

int64_t BvPermissionRequest::GetResources() {
  return delegate_->GetResources();
}

void BvPermissionRequest::CancelAndDelete() {
  processed_ = true;
  DeleteThis();
}

}  // namespace bison
