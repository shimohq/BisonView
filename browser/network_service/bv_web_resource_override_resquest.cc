#include "bison/browser/network_service/bv_web_resource_override_resquest.h"

#include <memory>
#include <utility>

#include "bison/bison_jni_headers/BvWebResourceOverrideRequest_jni.h"
#include "bison/browser/network_service/bv_web_resource_response.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"

#include "net/url_request/url_request.h"

using base::android::AppendJavaStringArrayToStringVector;
using base::android::ConvertJavaStringToUTF8;
using base::android::ScopedJavaLocalRef;

namespace bison {

BvWebResourceOverrideRequest::BvWebResourceOverrideRequest(
    const base::android::JavaRef<jobject>& obj)
    : java_object_(obj) {}

BvWebResourceOverrideRequest::~BvWebResourceOverrideRequest() = default;

bool BvWebResourceOverrideRequest::RaisedException(JNIEnv* env) const {
  return Java_BvWebResourceOverrideRequest_getRaisedException(env,
                                                                 java_object_);
}

bool BvWebResourceOverrideRequest::HasRequest(JNIEnv* env) const {
  return !Java_BvWebResourceOverrideRequest_getRequest(env, java_object_)
              .is_null();
}

std::string BvWebResourceOverrideRequest::GetRequestUrl(JNIEnv* env) const {
  ScopedJavaLocalRef<jstring> jstring_request_url =
      Java_BvWebResourceOverrideRequest_getRequestUrl(env, java_object_);
  return ConvertJavaStringToUTF8(jstring_request_url);
}

std::string BvWebResourceOverrideRequest::GetRequestMethod(
    JNIEnv* env) const {
  ScopedJavaLocalRef<jstring> jstring_request_method =
      Java_BvWebResourceOverrideRequest_getRequestMethod(env, java_object_);
  return ConvertJavaStringToUTF8(jstring_request_method);
}

void BvWebResourceOverrideRequest::GetRequestHeaders(
    JNIEnv* env,
    net::HttpRequestHeaders* headers) {
  ScopedJavaLocalRef<jobjectArray> jstringArray_headerNames =
      Java_BvWebResourceOverrideRequest_getRequestHeaderNames(env,
                                                                 java_object_);
  ScopedJavaLocalRef<jobjectArray> jstringArray_headerValues =
      Java_BvWebResourceOverrideRequest_getRequestHeaderValues(env,
                                                                  java_object_);
  if (jstringArray_headerNames.is_null() || jstringArray_headerValues.is_null())
    return;
  std::vector<std::string> header_names;
  std::vector<std::string> header_values;
  AppendJavaStringArrayToStringVector(env, jstringArray_headerNames,
                                      &header_names);
  AppendJavaStringArrayToStringVector(env, jstringArray_headerValues,
                                      &header_values);
  DCHECK_EQ(header_values.size(), header_names.size());
  for (size_t i = 0; i < header_names.size(); ++i) {
    headers->SetHeader(header_names[i], header_values[i]);
  }
}
// std::unique_ptr<BvWebResourceRequest>
// BvWebResourceOverrideRequest::GetRequest(JNIEnv* env) const {
//   ScopedJavaLocalRef<jobject> j_request =
//       Java_BvWebResourceOverrideRequest_getRequest(env, java_object_);
//   if (j_request.is_null())
//     return nullptr;
//   return std::make_unique<BvWebResourceRequest>(j_request);
// }

}  // namespace bison
