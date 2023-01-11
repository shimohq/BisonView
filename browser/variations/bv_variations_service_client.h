// create by jiang947

#ifndef BISON_BROWSER_VARIATIONS_BV_VARIATIONS_SERVICE_CLIENT_H_
#define BISON_BROWSER_VARIATIONS_BV_VARIATIONS_SERVICE_CLIENT_H_

#include <string>

#include "base/memory/scoped_refptr.h"
#include "components/variations/service/variations_service_client.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace bison {

// BvVariationsServiceClient provides an implementation of
// VariationsServiceClient, all members are currently stubs for WebView.
class BvVariationsServiceClient : public variations::VariationsServiceClient {
 public:
  BvVariationsServiceClient();

  BvVariationsServiceClient(const BvVariationsServiceClient&) = delete;
  BvVariationsServiceClient& operator=(const BvVariationsServiceClient&) =
      delete;

  ~BvVariationsServiceClient() override;

 private:
  base::Version GetVersionForSimulation() override;
  scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactory() override;
  network_time::NetworkTimeTracker* GetNetworkTimeTracker() override;
  version_info::Channel GetChannel() override;
  bool OverridesRestrictParameter(std::string* parameter) override;
  bool IsEnterprise() override;
};

}  // namespace bison

#endif  // BISON_BROWSER_VARIATIONS_BV_VARIATIONS_SERVICE_CLIENT_H_
