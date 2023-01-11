// create by jiang947

#ifndef BISON_BROWSER_ANDROID_PROTOCOL_HANDLER_H_
#define BISON_BROWSER_ANDROID_PROTOCOL_HANDLER_H_

#include <jni.h>
#include <memory>

class GURL;

namespace embedder_support {
class InputStream;
}

namespace bison {

std::unique_ptr<embedder_support::InputStream> CreateInputStream(
    JNIEnv* env,
    const GURL& url);

bool GetInputStreamMimeType(JNIEnv* env,
                            const GURL& url,
                            embedder_support::InputStream* stream,
                            std::string* mime_type);

}  // namespace bison

#endif  // BISON_BROWSER_ANDROID_PROTOCOL_HANDLER_H_
