
#include "bison/browser/network_service/bv_url_loader_throttle.h"

#include "bison/browser/bv_resource_context.h"
#include "bison/common/bv_features.h"

#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/redirect_info.h"
#include "services/network/public/cpp/resource_request.h"

namespace bison {

BvURLLoaderThrottle::BvURLLoaderThrottle(BvResourceContext* bv_resource_context)
    : bv_resource_context_(bv_resource_context) {}

BvURLLoaderThrottle::~BvURLLoaderThrottle() = default;

void BvURLLoaderThrottle::WillStartRequest(network::ResourceRequest* request,
                                           bool* defer) {
  AddExtraHeadersIfNeeded(request->url, &request->headers);
  if (!added_headers_.empty()) {
    original_origin_ = url::Origin::Create(request->url);
  }
}

void BvURLLoaderThrottle::WillRedirectRequest(
    net::RedirectInfo* redirect_info,
    const network::mojom::URLResponseHead& response_head,
    bool* defer,
    std::vector<std::string>* to_be_removed_request_headers,
    net::HttpRequestHeaders* modified_request_headers,
    net::HttpRequestHeaders* modified_cors_exempt_request_headers) {
  bool same_origin_only = base::FeatureList::IsEnabled(
      features::kWebViewExtraHeadersSameOriginOnly);

  if (!added_headers_.empty()) {
    bool is_same_origin =
        original_origin_.CanBeDerivedFrom(redirect_info->new_url);
    bool is_same_domain = net::registry_controlled_domains::SameDomainOrHost(
        redirect_info->new_url, original_origin_,
        net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

    if ((same_origin_only && !is_same_origin) || !is_same_domain) {
      // The headers we added must be removed.
      to_be_removed_request_headers->insert(
          to_be_removed_request_headers->end(),
          std::make_move_iterator(added_headers_.begin()),
          std::make_move_iterator(added_headers_.end()));
      added_headers_.clear();
    }
  }
}

void BvURLLoaderThrottle::AddExtraHeadersIfNeeded(
    const GURL& url,
    net::HttpRequestHeaders* headers) {
  std::string extra_headers = bv_resource_context_->GetExtraHeaders(url);
  if (extra_headers.empty())
    return;

  net::HttpRequestHeaders temp_headers;
  temp_headers.AddHeadersFromString(extra_headers);
  for (net::HttpRequestHeaders::Iterator it(temp_headers); it.GetNext();) {
    if (headers->HasHeader(it.name()))
      continue;

    headers->SetHeader(it.name(), it.value());
    added_headers_.push_back(it.name());
  }
}

}  // namespace bison
