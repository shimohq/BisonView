// create by jiang947 

#ifndef BISON_BROWSER_NETWORK_SERVICE_BISON_PROXY_CONFIG_MONITOR_H_
#define BISON_BROWSER_NETWORK_SERVICE_BISON_PROXY_CONFIG_MONITOR_H_


#include <memory>
#include <string>
#include <vector>

#include "mojo/public/cpp/bindings/remote_set.h"
#include "net/proxy_resolution/proxy_config_service_android.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace bison {

// This class configures proxy settings for NetworkContext if network service
// is enabled.
class BisonProxyConfigMonitor : public net::ProxyConfigService::Observer {
 public:
  BisonProxyConfigMonitor(const BisonProxyConfigMonitor&) = delete;
  BisonProxyConfigMonitor& operator=(const BisonProxyConfigMonitor&) = delete;

  static BisonProxyConfigMonitor* GetInstance();

  void AddProxyToNetworkContextParams(
      network::mojom::NetworkContextParams* network_context_params);
  std::string SetProxyOverride(
      const std::vector<net::ProxyConfigServiceAndroid::ProxyOverrideRule>&
          proxy_rules,
      const std::vector<std::string>& bypass_rules,
      base::OnceClosure callback);
  void ClearProxyOverride(base::OnceClosure callback);

 private:
  BisonProxyConfigMonitor();
  ~BisonProxyConfigMonitor() override;  

  friend class base::NoDestructor<BisonProxyConfigMonitor>;
  // net::ProxyConfigService::Observer implementation:
  void OnProxyConfigChanged(
      const net::ProxyConfigWithAnnotation& config,
      net::ProxyConfigService::ConfigAvailability availability) override;

  void FlushProxyConfig(base::OnceClosure callback);

  std::unique_ptr<net::ProxyConfigServiceAndroid> proxy_config_service_android_;
  mojo::RemoteSet<network::mojom::ProxyConfigClient> proxy_config_client_set_;
};

}  // namespace bison

#endif  // BISON_BROWSER_NETWORK_SERVICE_BISON_PROXY_CONFIG_MONITOR_H_
