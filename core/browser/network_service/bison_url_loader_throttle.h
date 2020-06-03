// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_BROWSER_NETWORK_SERVICE_BISON_URL_LOADER_THROTTLE_H_
#define BISON_CORE_BROWSER_NETWORK_SERVICE_BISON_URL_LOADER_THROTTLE_H_

#include "base/macros.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

class GURL;

namespace net {
class HttpRequestHeaders;
}

namespace bison {
class BisonResourceContext;

class BisonURLLoaderThrottle : public blink::URLLoaderThrottle {
 public:
  explicit BisonURLLoaderThrottle(BisonResourceContext* aw_resource_context);
  ~BisonURLLoaderThrottle() override;

  // blink::URLLoaderThrottle implementation:
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;
  void WillRedirectRequest(
      net::RedirectInfo* redirect_info,
      const network::ResourceResponseHead& response_head,
      bool* defer,
      std::vector<std::string>* to_be_removed_request_headers,
      net::HttpRequestHeaders* modified_request_headers) override;

 private:
  void AddExtraHeadersIfNeeded(const GURL& url,
                               net::HttpRequestHeaders* headers);

  BisonResourceContext* aw_resource_context_;

  DISALLOW_COPY_AND_ASSIGN(BisonURLLoaderThrottle);
};

}  // namespace bison

#endif  // BISON_CORE_BROWSER_NETWORK_SERVICE_BISON_URL_LOADER_THROTTLE_H_
