
#include "bison/browser/network_service/bison_url_loader_throttle.h"

#include "bison/browser/bison_resource_context.h"
#include "net/http/http_response_headers.h"

#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "net/http/http_request_headers.h"

namespace bison {

namespace {
enum class ExtraHeaders {
  kAddedInStartRequest = 0,
  kKeptOnSameOriginRedirect = 1,
  kRemovedOnCrossOriginRedirect = 2,
  kMaxValue = kRemovedOnCrossOriginRedirect
};

void RecordExtraHeadersUMA(ExtraHeaders value) {
  UMA_HISTOGRAM_ENUMERATION("BisonView.ExtraHeaders", value);
}

}  // namespace

BisonURLLoaderThrottle::BisonURLLoaderThrottle(
    BisonResourceContext* aw_resource_context)
    : aw_resource_context_(aw_resource_context) {}

BisonURLLoaderThrottle::~BisonURLLoaderThrottle() = default;

void BisonURLLoaderThrottle::WillStartRequest(network::ResourceRequest* request,
                                              bool* defer) {
  AddExtraHeadersIfNeeded(request->url, &request->headers);
  if (!added_headers_.empty()) {
    original_origin_ = url::Origin::Create(request->url);
    RecordExtraHeadersUMA(ExtraHeaders::kAddedInStartRequest);
  }
}

void BisonURLLoaderThrottle::WillRedirectRequest(
    net::RedirectInfo* redirect_info,
    const network::mojom::URLResponseHead& response_head,
    bool* defer,
    std::vector<std::string>* to_be_removed_request_headers,
    net::HttpRequestHeaders* modified_request_headers,
    net::HttpRequestHeaders* modified_cors_exempt_request_headers) {
  // bool same_origin_only = base::FeatureList::IsEnabled(
  //     features::kWebViewExtraHeadersSameOriginOnly);

  if (!added_headers_.empty()) {
    if (original_origin_.CanBeDerivedFrom(redirect_info->new_url)) {
      RecordExtraHeadersUMA(ExtraHeaders::kKeptOnSameOriginRedirect);
    } else {
      // Cross-origin redirect. Remove the headers we added if the feature is
      // enabled. added_headers_ is still cleared either way so that the metrics
      // will reflect what would have happened if the feature was enabled.
      // jiang
      // if (same_origin_only) {
      //   to_be_removed_request_headers->insert(
      //       to_be_removed_request_headers->end(),
      //       std::make_move_iterator(added_headers_.begin()),
      //       std::make_move_iterator(added_headers_.end()));
      // }
      added_headers_.clear();
      RecordExtraHeadersUMA(ExtraHeaders::kRemovedOnCrossOriginRedirect);
    }
  }

  // if (!same_origin_only) {
  //   AddExtraHeadersIfNeeded(redirect_info->new_url,
  //   modified_request_headers);
  // }
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
  for (net::HttpRequestHeaders::Iterator it(temp_headers); it.GetNext();) {
    if (headers->HasHeader(it.name()))
      continue;

    headers->SetHeader(it.name(), it.value());
    added_headers_.push_back(it.name());
  }
}

}  // namespace bison
