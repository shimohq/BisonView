// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/browser/bv_http_auth_handler.h"

#include <utility>

#include "bison/browser/bv_contents.h"
#include "bison/bison_jni_headers/BvHttpAuthHandler_jni.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/optional.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "net/base/auth.h"

using base::android::ConvertJavaStringToUTF16;
using base::android::JavaParamRef;
using content::BrowserThread;

namespace bison {

BvHttpAuthHandler::BvHttpAuthHandler(const net::AuthChallengeInfo& auth_info,
                                     content::WebContents* web_contents,
                                     bool first_auth_attempt,
                                     LoginAuthRequiredCallback callback)
    : WebContentsObserver(web_contents),
      host_(auth_info.challenger.host()),
      realm_(auth_info.realm),
      callback_(std::move(callback)) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = base::android::AttachCurrentThread();
  http_auth_handler_.Reset(Java_BvHttpAuthHandler_create(
      env, reinterpret_cast<intptr_t>(this), first_auth_attempt));

  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&BvHttpAuthHandler::Start, weak_factory_.GetWeakPtr()));
}

BvHttpAuthHandler::~BvHttpAuthHandler() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  Java_BvHttpAuthHandler_handlerDestroyed(base::android::AttachCurrentThread(),
                                          http_auth_handler_);
}

void BvHttpAuthHandler::Proceed(JNIEnv* env,
                                const JavaParamRef<jobject>& obj,
                                const JavaParamRef<jstring>& user,
                                const JavaParamRef<jstring>& password) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (callback_) {
    std::move(callback_).Run(
        net::AuthCredentials(ConvertJavaStringToUTF16(env, user),
                             ConvertJavaStringToUTF16(env, password)));
  }
}

void BvHttpAuthHandler::Cancel(JNIEnv* env, const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (callback_) {
    std::move(callback_).Run(base::nullopt);
  }
}

void BvHttpAuthHandler::Start() {
  DCHECK(web_contents());
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // The WebContents may have been destroyed during the PostTask.
  if (!web_contents()) {
    std::move(callback_).Run(base::nullopt);
    return;
  }

  BvContents* bv_contents = BvContents::FromWebContents(web_contents());
  if (!bv_contents->OnReceivedHttpAuthRequest(http_auth_handler_, host_,
                                              realm_)) {
    std::move(callback_).Run(base::nullopt);
  }
}

}  // namespace bison
