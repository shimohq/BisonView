
#include "android_protocol_handler.h"

#include <memory>
#include <utility>

#include "bison/bison_jni_headers/AndroidProtocolHandler_jni.h"
#include "bison/common/url_constants.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/jni_weak_ref.h"
#include "components/embedder_support/android/util/input_stream.h"
#include "content/public/common/url_constants.h"
#include "net/base/io_buffer.h"
#include "net/base/mime_util.h"
#include "net/base/net_errors.h"
#include "net/http/http_util.h"
#include "url/android/gurl_android.h"
#include "url/gurl.h"
#include "url/url_constants.h"

using base::android::AttachCurrentThread;
using base::android::ClearException;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;
using embedder_support::InputStream;

namespace bison {

// static
std::unique_ptr<InputStream> CreateInputStream(JNIEnv* env, const GURL& url) {
  DCHECK(url.is_valid());
  DCHECK(env);

  // Open the input stream.
  ScopedJavaLocalRef<jobject> stream =
      bison::Java_AndroidProtocolHandler_open(
          env, url::GURLAndroid::FromNativeGURL(env, url));

  if (!stream) {
    DLOG(ERROR) << "Unable to open input stream for Android URL";
    return nullptr;
  }
  return std::make_unique<InputStream>(stream);
}

bool GetInputStreamMimeType(JNIEnv* env,
                            const GURL& url,
                            embedder_support::InputStream* stream,
                            std::string* mime_type) {
  // Query the mime type from the Java side. It is possible for the query to
  // fail, as the mime type cannot be determined for all supported schemes.
  ScopedJavaLocalRef<jstring> returned_type =
      bison::Java_AndroidProtocolHandler_getMimeType(
          env, stream->jobj(), url::GURLAndroid::FromNativeGURL(env, url));
  if (!returned_type)
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
