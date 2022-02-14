// create by jiang947

#ifndef BISON_BROWSER_NETWORK_SERVICE_BISON_URL_LOADER_THROTTLE_H_
#define BISON_BROWSER_NETWORK_SERVICE_BISON_URL_LOADER_THROTTLE_H_

#include "base/macros.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

class GURL;

namespace net {
class HttpRequestHeaders;
}

namespace bison {
class BvResourceContext;

class BisonURLLoaderThrottle : public blink::URLLoaderThrottle {
 public:
  explicit BisonURLLoaderThrottle(BvResourceContext* aw_resource_context);
  ~BisonURLLoaderThrottle() override;

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

  BvResourceContext* aw_resource_context_;
  std::vector<std::string> added_headers_;
  url::Origin original_origin_;

  DISALLOW_COPY_AND_ASSIGN(BisonURLLoaderThrottle);
};

}  // namespace bison

#endif  // BISON_BROWSER_NETWORK_SERVICE_BISON_URL_LOADER_THROTTLE_H_
