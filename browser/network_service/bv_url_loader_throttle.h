// create by jiang947

#ifndef BISON_BROWSER_NETWORK_SERVICE_BV_URL_LOADER_THROTTLE_H_
#define BISON_BROWSER_NETWORK_SERVICE_BV_URL_LOADER_THROTTLE_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"
#include "url/origin.h"

class GURL;

namespace net {
class HttpRequestHeaders;
}

namespace bison {
class BvResourceContext;

class BvURLLoaderThrottle : public blink::URLLoaderThrottle {
 public:
  explicit BvURLLoaderThrottle(BvResourceContext* bv_resource_context);

  BvURLLoaderThrottle(const BvURLLoaderThrottle&) = delete;
  BvURLLoaderThrottle& operator=(const BvURLLoaderThrottle&) = delete;

  ~BvURLLoaderThrottle() override;

  // blink::URLLoaderThrottle implementation:
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;
  void WillRedirectRequest(
      net::RedirectInfo* redirect_info,
      const network::mojom::URLResponseHead& response_head,
      bool* defer,
      std::vector<std::string>* to_be_removed_request_headers,
      net::HttpRequestHeaders* modified_request_headers,
      net::HttpRequestHeaders* modified_cors_exempt_request_headers) override;

 private:
  void AddExtraHeadersIfNeeded(const GURL& url,
                               net::HttpRequestHeaders* headers);

  raw_ptr<BvResourceContext> bv_resource_context_;
  std::vector<std::string> added_headers_;
  url::Origin original_origin_;
};

}  // namespace bison

#endif  // BISON_BROWSER_NETWORK_SERVICE_BV_URL_LOADER_THROTTLE_H_
