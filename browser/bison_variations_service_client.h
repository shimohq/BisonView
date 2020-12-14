// create by jiang947 


#ifndef BISON_BROWSER_BISON_VARIATIONS_SERVICE_CLIENT_H_
#define BISON_BROWSER_BISON_VARIATIONS_SERVICE_CLIENT_H_


#include <string>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "components/variations/service/variations_service_client.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace bison {

// BisonVariationsServiceClient provides an implementation of
// VariationsServiceClient, all members are currently stubs for WebView.
class BisonVariationsServiceClient : public variations::VariationsServiceClient {
 public:
  BisonVariationsServiceClient();
  ~BisonVariationsServiceClient() override;

 private:
  VersionCallback GetVersionForSimulationCallback()override;
  scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactory() override;
  network_time::NetworkTimeTracker* GetNetworkTimeTracker() override;
  version_info::Channel GetChannel() override;
  bool OverridesRestrictParameter(std::string* parameter) override;
  bool IsEnterprise() override;

  DISALLOW_COPY_AND_ASSIGN(BisonVariationsServiceClient);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_VARIATIONS_SERVICE_CLIENT_H_
