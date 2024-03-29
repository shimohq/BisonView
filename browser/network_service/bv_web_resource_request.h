// create by jiang947

#ifndef BISON_BROWSER_NETWORK_SERVICE_BV_WEB_RESOURCE_REQUEST_H_
#define BISON_BROWSER_NETWORK_SERVICE_BV_WEB_RESOURCE_REQUEST_H_

#include <string>
#include <vector>

#include "base/android/scoped_java_ref.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

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
struct BvWebResourceRequest final {
  explicit BvWebResourceRequest(const network::ResourceRequest& request);
  BvWebResourceRequest(const std::string& in_url,
                       const std::string& in_method,
                       bool in_is_outermost_main_frame,
                       bool in_has_user_gesture,
                       const net::HttpRequestHeaders& in_headers);

  // Add default copy/move/assign operators. Adding explicit destructor
  // prevents generating move operator.
  BvWebResourceRequest(BvWebResourceRequest&& other);
  BvWebResourceRequest& operator=(BvWebResourceRequest&& other);
  ~BvWebResourceRequest();

  // The java equivalent
  struct BisonJavaWebResourceRequest {
    BisonJavaWebResourceRequest();
    ~BisonJavaWebResourceRequest();

    base::android::ScopedJavaLocalRef<jstring> jurl;
    base::android::ScopedJavaLocalRef<jstring> jmethod;
    base::android::ScopedJavaLocalRef<jobjectArray> jheader_names;
    base::android::ScopedJavaLocalRef<jobjectArray> jheader_values;
  };

  // Convenience method to convert BvWebResourceRequest to Java equivalent.
  static void ConvertToJava(JNIEnv* env,
                            const BvWebResourceRequest& request,
                            BisonJavaWebResourceRequest* jRequest);

  std::string url;
  std::string method;
  bool is_outermost_main_frame;
  bool has_user_gesture;
  std::vector<std::string> header_names;
  std::vector<std::string> header_values;
  absl::optional<bool> is_renderer_initiated;
};

}  // namespace bison

#endif  // BISON_BROWSER_NETWORK_SERVICE_BV_WEB_RESOURCE_REQUEST_H_
