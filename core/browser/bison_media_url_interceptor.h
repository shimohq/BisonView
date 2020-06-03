// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_BROWSER_BISON_MEDIA_URL_INTERCEPTOR_H_
#define BISON_CORE_BROWSER_BISON_MEDIA_URL_INTERCEPTOR_H_

#include <stdint.h>

#include <string>

#include "base/android/jni_android.h"
#include "media/base/android/media_url_interceptor.h"

namespace bison {

// Interceptor to handle urls for media assets in the apk.
class BisonMediaUrlInterceptor : public media::MediaUrlInterceptor {
 public:
  bool Intercept(const std::string& url,
                 int* fd,
                 int64_t* offset,
                 int64_t* size) const override;
};

}  // namespace bison

#endif  // BISON_CORE_BROWSER_BISON_MEDIA_URL_INTERCEPTOR_H_
