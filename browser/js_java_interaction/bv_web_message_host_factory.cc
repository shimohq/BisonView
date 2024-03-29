// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/browser/js_java_interaction/bv_web_message_host_factory.h"

#include "bison/browser/js_java_interaction/js_reply_proxy.h"
#include "bison/bison_jni_headers/WebMessageListenerHolder_jni.h"
#include "bison/bison_jni_headers/WebMessageListenerInfo_jni.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "components/js_injection/browser/js_communication_host.h"
#include "components/js_injection/browser/web_message.h"
#include "components/js_injection/browser/web_message_host.h"
#include "components/js_injection/common/origin_matcher.h"
#include "content/public/browser/android/app_web_message_port.h"

namespace bison {
namespace {

// Calls through to WebMessageListenerInfo.
class BvWebMessageHost : public js_injection::WebMessageHost {
 public:
  BvWebMessageHost(js_injection::WebMessageReplyProxy* reply_proxy,
                   const base::android::ScopedJavaGlobalRef<jobject>& listener,
                   const std::string& origin_string,
                   bool is_main_frame)
      : reply_proxy_(reply_proxy),
        listener_(listener),
        origin_string_(origin_string),
        is_main_frame_(is_main_frame) {}

  ~BvWebMessageHost() override = default;

  // js_injection::WebMessageHost:
  void OnPostMessage(
      std::unique_ptr<js_injection::WebMessage> message) override {
    JNIEnv* env = base::android::AttachCurrentThread();
    base::android::ScopedJavaGlobalRef<jobjectArray> jports =
        content::AppWebMessagePort::WrapJavaArray(env,
                                                  std::move(message->ports));
    Java_WebMessageListenerHolder_onPostMessage(
        env, listener_,
        base::android::ConvertUTF16ToJavaString(env, message->message),
        base::android::ConvertUTF8ToJavaString(env, origin_string_),
        is_main_frame_, jports, reply_proxy_.GetJavaPeer());
  }

 private:
  JsReplyProxy reply_proxy_;
  base::android::ScopedJavaGlobalRef<jobject> listener_;
  const std::string origin_string_;
  const bool is_main_frame_;
};

}  // namespace

BvWebMessageHostFactory::BvWebMessageHostFactory(
    const base::android::JavaParamRef<jobject>& listener)
    : listener_(listener) {}

BvWebMessageHostFactory::~BvWebMessageHostFactory() = default;

// static
base::android::ScopedJavaLocalRef<jobjectArray>
BvWebMessageHostFactory::GetWebMessageListenerInfo(
    js_injection::JsCommunicationHost* host,
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& clazz) {
  auto factories = host->GetWebMessageHostFactories();
  jobjectArray joa =
      env->NewObjectArray(factories.size(), clazz.obj(), nullptr);
  base::android::CheckException(env);

  for (size_t i = 0; i < factories.size(); ++i) {
    const auto& factory = factories[i];
    const std::vector<std::string> rules =
        factory.allowed_origin_rules.Serialize();
    base::android::ScopedJavaLocalRef<jobject> object =
        Java_WebMessageListenerInfo_create(
            env, base::android::ConvertUTF16ToJavaString(env, factory.js_name),
            base::android::ToJavaArrayOfStrings(env, rules),
            static_cast<BvWebMessageHostFactory*>(factory.factory)->listener_);
    env->SetObjectArrayElement(joa, i, object.obj());
  }
  return base::android::ScopedJavaLocalRef<jobjectArray>(env, joa);
}

std::unique_ptr<js_injection::WebMessageHost>
BvWebMessageHostFactory::CreateHost(const std::string& origin_string,
                                    bool is_main_frame,
                                    js_injection::WebMessageReplyProxy* proxy) {
  return std::make_unique<BvWebMessageHost>(proxy, listener_, origin_string,
                                            is_main_frame);
}

}  // namespace bison
