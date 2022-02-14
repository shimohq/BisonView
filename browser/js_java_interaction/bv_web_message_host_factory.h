// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_BROWSER_JS_JAVA_INTERACTION_AW_WEB_MESSAGE_HOST_FACTORY_H_
#define BISON_BROWSER_JS_JAVA_INTERACTION_AW_WEB_MESSAGE_HOST_FACTORY_H_

#include "base/android/scoped_java_ref.h"
#include "components/js_injection/browser/web_message_host_factory.h"

namespace js_injection {
class JsCommunicationHost;
}

namespace bison {

// Adapts WebMessageHostFactory for use by WebView. An BvWebMessageHostFactory
// is created per WebMessageListener. More specifically, every call to
// AwContents::AddWebMessageListener() creates a new BvWebMessageHostFactory.
class BvWebMessageHostFactory : public js_injection::WebMessageHostFactory {
 public:
  explicit BvWebMessageHostFactory(
      const base::android::JavaParamRef<jobject>& listener);
  BvWebMessageHostFactory(const BvWebMessageHostFactory&) = delete;
  BvWebMessageHostFactory& operator=(const BvWebMessageHostFactory&) = delete;
  ~BvWebMessageHostFactory() override;

  // Returns an array of WebMessageListenerInfos based on the registered
  // factories.
  static base::android::ScopedJavaLocalRef<jobjectArray>
  GetWebMessageListenerInfo(js_injection::JsCommunicationHost* host,
                            JNIEnv* env,
                            const base::android::JavaParamRef<jclass>& clazz);

  // js_injection::WebMessageConnection:
  std::unique_ptr<js_injection::WebMessageHost> CreateHost(
      const std::string& origin_string,
      bool is_main_frame,
      js_injection::WebMessageReplyProxy* proxy) override;

 private:
  // The WebMessageListenerHost that was supplied to
  // AwContents::AddWebMessageListener().
  base::android::ScopedJavaGlobalRef<jobject> listener_;
};

}  // namespace bison

#endif  // BISON_BROWSER_JS_JAVA_INTERACTION_AW_WEB_MESSAGE_HOST_FACTORY_H_
