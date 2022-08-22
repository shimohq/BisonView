#ifndef BISON_BROWSER_NETWORK_SERVICE_BV_PROXY_CONFIG_MONITOR_H_
#define BISON_BROWSER_NETWORK_SERVICE_BV_PROXY_CONFIG_MONITOR_H_




#include <memory>
#include <string>
#include <vector>

#include "base/no_destructor.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "net/proxy_resolution/proxy_config_service_android.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace network {
namespace mojom {
class ProxyConfigClient;
}
}  // namespace network

namespace bison {

// This class configures proxy settings for NetworkContext if network service
// is enabled.
class BvProxyConfigMonitor : public net::ProxyConfigService::Observer {
 public:
  BvProxyConfigMonitor(const BvProxyConfigMonitor&) = delete;
  BvProxyConfigMonitor& operator=(const BvProxyConfigMonitor&) = delete;

  static BvProxyConfigMonitor* GetInstance();

  void AddProxyToNetworkContextParams(
      network::mojom::NetworkContextParams* network_context_params);
  std::string SetProxyOverride(
      const std::vector<net::ProxyConfigServiceAndroid::ProxyOverrideRule>&
          proxy_rules,
      const std::vector<std::string>& bypass_rules,
      const bool reverse_bypass,
      base::OnceClosure callback);
  void ClearProxyOverride(base::OnceClosure callback);

 private:
  BvProxyConfigMonitor();
  ~BvProxyConfigMonitor() override;

  friend class base::NoDestructor<BvProxyConfigMonitor>;
  // net::ProxyConfigService::Observer implementation:
  void OnProxyConfigChanged(
      const net::ProxyConfigWithAnnotation& config,
      net::ProxyConfigService::ConfigAvailability availability) override;

  void FlushProxyConfig(base::OnceClosure callback);

  std::unique_ptr<net::ProxyConfigServiceAndroid> proxy_config_service_android_;
  mojo::RemoteSet<network::mojom::ProxyConfigClient> proxy_config_client_set_;
};

}  // namespace bison

#endif  // BISON_BROWSER_NETWORK_SERVICE_BV_PROXY_CONFIG_MONITOR_H_
