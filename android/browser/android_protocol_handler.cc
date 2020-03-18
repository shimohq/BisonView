// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/browser/android_protocol_handler.h"

#include <memory>
#include <utility>

#include "bison/android/browser/input_stream.h"
#include "bison/android/browser_jni_headers/AndroidProtocolHandler_jni.h"
#include "bison/android/common/url_constants.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/jni_weak_ref.h"
#include "content/public/common/url_constants.h"
#include "net/base/io_buffer.h"
#include "net/base/mime_util.h"
#include "net/base/net_errors.h"
#include "net/http/http_util.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"
#include "url/url_constants.h"

using bison::InputStream;
using bison::InputStream;
using base::android::AttachCurrentThread;
using base::android::ClearException;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;

namespace bison {

// static
std::unique_ptr<InputStream> CreateInputStream(JNIEnv* env, const GURL& url) {
  DCHECK(url.is_valid());
  DCHECK(env);

  // Open the input stream.
  ScopedJavaLocalRef<jstring> jurl = ConvertUTF8ToJavaString(env, url.spec());
  ScopedJavaLocalRef<jobject> stream =
      bison::Java_AndroidProtocolHandler_open(env, jurl);

  if (stream.is_null()) {
    DLOG(ERROR) << "Unable to open input stream for Android URL";
    return nullptr;
  }
  return std::make_unique<InputStream>(stream);
}

bool GetInputStreamMimeType(JNIEnv* env,
                            const GURL& url,
                            InputStream* stream,
                            std::string* mime_type) {
  // Query the mime type from the Java side. It is possible for the query to
  // fail, as the mime type cannot be determined for all supported schemes.
  ScopedJavaLocalRef<jstring> java_url =
      ConvertUTF8ToJavaString(env, url.spec());
  ScopedJavaLocalRef<jstring> returned_type =
      bison::Java_AndroidProtocolHandler_getMimeType(
          env, stream->jobj(), java_url);
  if (returned_type.is_null())
    return false;

  *mime_type = base::android::ConvertJavaStringToUTF8(returned_type);
  return true;
}

static ScopedJavaLocalRef<jstring>
JNI_AndroidProtocolHandler_GetAndroidAssetPath(JNIEnv* env) {
  return ConvertUTF8ToJavaString(env, bison::kAndroidAssetPath);
}

static ScopedJavaLocalRef<jstring>
JNI_AndroidProtocolHandler_GetAndroidResourcePath(JNIEnv* env) {
  return ConvertUTF8ToJavaString(env, bison::kAndroidResourcePath);
}

}  // namespace bison
