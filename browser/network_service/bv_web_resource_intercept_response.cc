#include "bison/browser/network_service/bv_web_resource_intercept_response.h"

#include <memory>
#include <utility>

#include "bison/bison_jni_headers/BvWebResourceInterceptResponse_jni.h"

#include "base/android/jni_android.h"
#include "components/embedder_support/android/util/web_resource_response.h"

using base::android::ScopedJavaLocalRef;

namespace bison {

BvWebResourceInterceptResponse::BvWebResourceInterceptResponse(
    const base::android::JavaRef<jobject>& obj)
    : java_object_(obj) {}

BvWebResourceInterceptResponse::~BvWebResourceInterceptResponse() = default;

bool BvWebResourceInterceptResponse::RaisedException(JNIEnv* env) const {
  return Java_BvWebResourceInterceptResponse_getRaisedException(env,
                                                                java_object_);
}

bool BvWebResourceInterceptResponse::HasResponse(JNIEnv* env) const {
  return !!Java_BvWebResourceInterceptResponse_getResponse(env, java_object_);
}

std::unique_ptr<embedder_support::WebResourceResponse>
BvWebResourceInterceptResponse::GetResponse(JNIEnv* env) const {
  ScopedJavaLocalRef<jobject> j_response =
      Java_BvWebResourceInterceptResponse_getResponse(env, java_object_);
  if (!j_response)
    return nullptr;
  return std::make_unique<embedder_support::WebResourceResponse>(j_response);
}

}  // namespace bison
