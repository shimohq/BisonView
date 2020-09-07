// create by jiang947

#ifndef BISON_BROWSER_ANDROID_PROTOCOL_HANDLER_H_
#define BISON_BROWSER_ANDROID_PROTOCOL_HANDLER_H_

#include <jni.h>

#include <memory>

class GURL;

namespace bison {
class InputStream;

std::unique_ptr<InputStream> CreateInputStream(JNIEnv* env, const GURL& url);

bool GetInputStreamMimeType(JNIEnv* env,
                            const GURL& url,
                            InputStream* stream,
                            std::string* mime_type);

}  // namespace bison

#endif  // BISON_BROWSER_ANDROID_PROTOCOL_HANDLER_H_