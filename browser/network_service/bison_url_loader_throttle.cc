
#include "bison/browser/network_service/bison_url_loader_throttle.h"

#include "bison/browser/bison_resource_context.h"
#include "net/http/http_response_headers.h"

namespace bison {

BisonURLLoaderThrottle::BisonURLLoaderThrottle(BisonResourceContext* aw_resource_context)
    : aw_resource_context_(aw_resource_context) {}

BisonURLLoaderThrottle::~BisonURLLoaderThrottle() = default;

void BisonURLLoaderThrottle::WillStartRequest(network::ResourceRequest* request,
                                           bool* defer) {
  AddExtraHeadersIfNeeded(request->url, &request->headers);
}

void BisonURLLoaderThrottle::WillRedirectRequest(
    net::RedirectInfo* redirect_info,
    const network::ResourceResponseHead& response_head,
    bool* defer,
    std::vector<std::string>* to_be_removed_request_headers,
    net::HttpRequestHeaders* modified_request_headers) {
  AddExtraHeadersIfNeeded(redirect_info->new_url, modified_request_headers);
}

void BisonURLLoaderThrottle::AddExtraHeadersIfNeeded(
    const GURL& url,
    net::HttpRequestHeaders* headers) {
  std::string extra_headers = aw_resource_context_->GetExtraHeaders(url);
  if (extra_headers.empty())
    return;

  net::HttpRequestHeaders temp_headers;
  temp_headers.AddHeadersFromString(extra_headers);
  for (net::HttpRequestHeaders::Iterator it(temp_headers); it.GetNext();)
    headers->SetHeaderIfMissing(it.name(), it.value());
}

}  // namespace bison
