#include "bison/browser/network_service/bv_web_resource_response.h"

#include <memory>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "bison/bison_jni_headers/BvWebResourceResponse_jni.h"
#include "bison/browser/input_stream.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_job.h"

using base::android::AppendJavaStringArrayToStringVector;
using base::android::ScopedJavaLocalRef;

namespace bison {

BvWebResourceResponse::BvWebResourceResponse(
    const base::android::JavaRef<jobject>& obj)
    : java_object_(obj), input_stream_transferred_(false) {}

BvWebResourceResponse::~BvWebResourceResponse() {}

bool BvWebResourceResponse::HasInputStream(JNIEnv* env) const {
  ScopedJavaLocalRef<jobject> jstream =
      Java_BvWebResourceResponse_getData(env, java_object_);
  return !jstream.is_null();
}

std::unique_ptr<InputStream> BvWebResourceResponse::GetInputStream(
    JNIEnv* env) {
  // Only allow to call GetInputStream once per object, because this method
  // transfers ownership of the stream and once the unique_ptr<InputStream>
  // is deleted it also closes the original java input stream. This
  // side-effect can result in unexpected behavior, e.g. trying to read
  // from a closed stream.
  DCHECK(!input_stream_transferred_);

  if (input_stream_transferred_)
    return nullptr;

  input_stream_transferred_ = true;
  ScopedJavaLocalRef<jobject> jstream =
      Java_BvWebResourceResponse_getData(env, java_object_);
  if (jstream.is_null())
    return nullptr;
  return std::make_unique<InputStream>(jstream);
}

bool BvWebResourceResponse::GetMimeType(JNIEnv* env,
                                           std::string* mime_type) const {
  ScopedJavaLocalRef<jstring> jstring_mime_type =
      Java_BvWebResourceResponse_getMimeType(env, java_object_);
  if (jstring_mime_type.is_null())
    return false;
  *mime_type = ConvertJavaStringToUTF8(jstring_mime_type);
  return true;
}

bool BvWebResourceResponse::GetCharset(JNIEnv* env,
                                          std::string* charset) const {
  ScopedJavaLocalRef<jstring> jstring_charset =
      Java_BvWebResourceResponse_getCharset(env, java_object_);
  if (jstring_charset.is_null())
    return false;
  *charset = ConvertJavaStringToUTF8(jstring_charset);
  return true;
}

bool BvWebResourceResponse::GetStatusInfo(JNIEnv* env,
                                             int* status_code,
                                             std::string* reason_phrase) const {
  int status = Java_BvWebResourceResponse_getStatusCode(env, java_object_);
  ScopedJavaLocalRef<jstring> jstring_reason_phrase =
      Java_BvWebResourceResponse_getReasonPhrase(env, java_object_);
  if (status < 100 || status >= 600 || jstring_reason_phrase.is_null())
    return false;
  *status_code = status;
  *reason_phrase = ConvertJavaStringToUTF8(jstring_reason_phrase);
  return true;
}

bool BvWebResourceResponse::GetResponseHeaders(
    JNIEnv* env,
    net::HttpResponseHeaders* headers) const {
  ScopedJavaLocalRef<jobjectArray> jstringArray_headerNames =
      Java_BvWebResourceResponse_getResponseHeaderNames(env, java_object_);
  ScopedJavaLocalRef<jobjectArray> jstringArray_headerValues =
      Java_BvWebResourceResponse_getResponseHeaderValues(env, java_object_);
  if (jstringArray_headerNames.is_null() || jstringArray_headerValues.is_null())
    return false;
  std::vector<std::string> header_names;
  std::vector<std::string> header_values;
  AppendJavaStringArrayToStringVector(env, jstringArray_headerNames,
                                      &header_names);
  AppendJavaStringArrayToStringVector(env, jstringArray_headerValues,
                                      &header_values);
  DCHECK_EQ(header_values.size(), header_names.size());
  for (size_t i = 0; i < header_names.size(); ++i) {
    headers->AddHeader(header_names[i], header_values[i]);
  }
  return true;
}

}  // namespace bison
