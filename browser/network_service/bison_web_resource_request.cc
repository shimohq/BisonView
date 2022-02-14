#include "bison/browser/network_service/bison_web_resource_request.h"

#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "services/network/public/cpp/resource_request.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "ui/base/page_transition_types.h"

using base::android::ConvertJavaStringToUTF16;
using base::android::ConvertUTF16ToJavaString;
using base::android::ConvertUTF8ToJavaString;
using base::android::ToJavaArrayOfStrings;

namespace bison {
namespace {

void ConvertRequestHeadersToVectors(const net::HttpRequestHeaders& headers,
                                    std::vector<std::string>* header_names,
                                    std::vector<std::string>* header_values) {
  DCHECK(header_names->empty());
  DCHECK(header_values->empty());
  net::HttpRequestHeaders::Iterator headers_iterator(headers);
  while (headers_iterator.GetNext()) {
    header_names->push_back(headers_iterator.name());
    header_values->push_back(headers_iterator.value());
  }
}

}  // namespace

BisonWebResourceRequest::BisonWebResourceRequest(
    const network::ResourceRequest& request)
    : url(request.url.spec()),
      method(request.method),
      is_main_frame(request.resource_type ==
                    static_cast<int>(blink::mojom::ResourceType::kMainFrame)),
      has_user_gesture(request.has_user_gesture),
      is_renderer_initiated(ui::PageTransitionIsWebTriggerable(
          static_cast<ui::PageTransition>(request.transition_type))) {
  ConvertRequestHeadersToVectors(request.headers, &header_names,
                                 &header_values);
}

BisonWebResourceRequest::BisonWebResourceRequest(
    const std::string& in_url,
    const std::string& in_method,
    bool in_is_main_frame,
    bool in_has_user_gesture,
    const net::HttpRequestHeaders& in_headers)
    : url(in_url),
      method(in_method),
      is_main_frame(in_is_main_frame),
      has_user_gesture(in_has_user_gesture) {
  ConvertRequestHeadersToVectors(in_headers, &header_names, &header_values);
}

BisonWebResourceRequest::BisonWebResourceRequest(
    BisonWebResourceRequest&& other) = default;
BisonWebResourceRequest& BisonWebResourceRequest::operator=(
    BisonWebResourceRequest&& other) = default;
BisonWebResourceRequest::~BisonWebResourceRequest() = default;

BisonWebResourceRequest::BisonJavaWebResourceRequest::
    BisonJavaWebResourceRequest() = default;
BisonWebResourceRequest::BisonJavaWebResourceRequest::
    ~BisonJavaWebResourceRequest() = default;

// static
void BisonWebResourceRequest::ConvertToJava(
    JNIEnv* env,
    const BisonWebResourceRequest& request,
    BisonJavaWebResourceRequest* jRequest) {
  jRequest->jurl = ConvertUTF8ToJavaString(env, request.url);
  jRequest->jmethod = ConvertUTF8ToJavaString(env, request.method);
  jRequest->jheader_names = ToJavaArrayOfStrings(env, request.header_names);
  jRequest->jheader_values = ToJavaArrayOfStrings(env, request.header_values);
}

}  // namespace bison
