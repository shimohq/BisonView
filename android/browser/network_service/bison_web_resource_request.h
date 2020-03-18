// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_BROWSER_NETWORK_SERVICE_BISON_WEB_RESOURCE_REQUEST_H_
#define BISON_ANDROID_BROWSER_NETWORK_SERVICE_BISON_WEB_RESOURCE_REQUEST_H_

#include <string>
#include <vector>

#include "base/android/scoped_java_ref.h"
#include "base/optional.h"

namespace net {
class HttpRequestHeaders;
}

namespace network {
struct ResourceRequest;
}

namespace bison {

// A passive data structure only used to carry request information. This
// class should be copyable.
// The fields are ultimately guided by android.webkit.WebResourceRequest:
// https://developer.android.com/reference/android/webkit/WebResourceRequest.html
struct BisonWebResourceRequest final {
  explicit BisonWebResourceRequest(const network::ResourceRequest& request);
  BisonWebResourceRequest(const std::string& in_url,
                       const std::string& in_method,
                       bool in_is_main_frame,
                       bool in_has_user_gesture,
                       const net::HttpRequestHeaders& in_headers);

  // Add default copy/move/assign operators. Adding explicit destructor
  // prevents generating move operator.
  BisonWebResourceRequest(BisonWebResourceRequest&& other);
  BisonWebResourceRequest& operator=(BisonWebResourceRequest&& other);
  ~BisonWebResourceRequest();

  // The java equivalent
  struct BisonJavaWebResourceRequest {
    BisonJavaWebResourceRequest();
    ~BisonJavaWebResourceRequest();

    base::android::ScopedJavaLocalRef<jstring> jurl;
    base::android::ScopedJavaLocalRef<jstring> jmethod;
    base::android::ScopedJavaLocalRef<jobjectArray> jheader_names;
    base::android::ScopedJavaLocalRef<jobjectArray> jheader_values;
  };

  // Convenience method to convert BisonWebResourceRequest to Java equivalent.
  static void ConvertToJava(JNIEnv* env,
                            const BisonWebResourceRequest& request,
                            BisonJavaWebResourceRequest* jRequest);

  std::string url;
  std::string method;
  bool is_main_frame;
  bool has_user_gesture;
  std::vector<std::string> header_names;
  std::vector<std::string> header_values;
  base::Optional<bool> is_renderer_initiated;
};

}  // namespace bison

#endif  // BISON_ANDROID_BROWSER_NETWORK_SERVICE_BISON_WEB_RESOURCE_REQUEST_H_
