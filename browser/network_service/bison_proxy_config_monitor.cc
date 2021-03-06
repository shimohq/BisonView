#include "bison/browser/network_service/bison_proxy_config_monitor.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/barrier_closure.h"
#include "base/bind.h"
#include "base/no_destructor.h"
#include "base/trace_event/trace_event.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace bison {

namespace {
const char kProxyServerSwitch[] = "proxy-server";
const char kProxyBypassListSwitch[] = "proxy-bypass-list";
}  // namespace

BisonProxyConfigMonitor::BisonProxyConfigMonitor() {
  proxy_config_service_android_ =
      std::make_unique<net::ProxyConfigServiceAndroid>(
          base::ThreadTaskRunnerHandle::Get(),
          base::ThreadTaskRunnerHandle::Get());
  proxy_config_service_android_->set_exclude_pac_url(true);
  proxy_config_service_android_->AddObserver(this);
}

BisonProxyConfigMonitor::~BisonProxyConfigMonitor() {
  proxy_config_service_android_->RemoveObserver(this);
}

BisonProxyConfigMonitor* BisonProxyConfigMonitor::GetInstance() {
  static base::NoDestructor<BisonProxyConfigMonitor> instance;
  return instance.get();
}

void BisonProxyConfigMonitor::AddProxyToNetworkContextParams(
    network::mojom::NetworkContextParams* network_context_params) {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(kProxyServerSwitch)) {
    std::string proxy = command_line.GetSwitchValueASCII(kProxyServerSwitch);
    net::ProxyConfig proxy_config;
    proxy_config.proxy_rules().ParseFromString(proxy);
    if (command_line.HasSwitch(kProxyBypassListSwitch)) {
      std::string bypass_list =
          command_line.GetSwitchValueASCII(kProxyBypassListSwitch);
      proxy_config.proxy_rules().bypass_rules.ParseFromString(bypass_list);
    }

    network_context_params->initial_proxy_config =
        net::ProxyConfigWithAnnotation(proxy_config, NO_TRAFFIC_ANNOTATION_YET);
  } else {
    mojo::PendingRemote<network::mojom::ProxyConfigClient> proxy_config_client;
    network_context_params->proxy_config_client_receiver =
        proxy_config_client.InitWithNewPipeAndPassReceiver();
    proxy_config_client_set_.Add(std::move(proxy_config_client));

    net::ProxyConfigWithAnnotation proxy_config;
    net::ProxyConfigService::ConfigAvailability availability =
        proxy_config_service_android_->GetLatestProxyConfig(&proxy_config);
    if (availability == net::ProxyConfigService::CONFIG_VALID)
      network_context_params->initial_proxy_config = proxy_config;
  }
}

void BisonProxyConfigMonitor::OnProxyConfigChanged(
    const net::ProxyConfigWithAnnotation& config,
    net::ProxyConfigService::ConfigAvailability availability) {
  for (const auto& proxy_config_client : proxy_config_client_set_) {
    switch (availability) {
      case net::ProxyConfigService::CONFIG_VALID:
        proxy_config_client->OnProxyConfigUpdated(config);
        break;
      case net::ProxyConfigService::CONFIG_UNSET:
        proxy_config_client->OnProxyConfigUpdated(
            net::ProxyConfigWithAnnotation::CreateDirect());
        break;
      case net::ProxyConfigService::CONFIG_PENDING:
        NOTREACHED();
        break;
    }
  }
}

std::string BisonProxyConfigMonitor::SetProxyOverride(
    const std::vector<net::ProxyConfigServiceAndroid::ProxyOverrideRule>&
        proxy_rules,
    const std::vector<std::string>& bypass_rules,
    base::OnceClosure callback) {
  return proxy_config_service_android_->SetProxyOverride(
      proxy_rules, bypass_rules,
      base::BindOnce(&BisonProxyConfigMonitor::FlushProxyConfig,
                     base::Unretained(this), std::move(callback)));
}

void BisonProxyConfigMonitor::ClearProxyOverride(base::OnceClosure callback) {
  proxy_config_service_android_->ClearProxyOverride(
      base::BindOnce(&BisonProxyConfigMonitor::FlushProxyConfig,
                     base::Unretained(this), std::move(callback)));
}

void BisonProxyConfigMonitor::FlushProxyConfig(base::OnceClosure callback) {
  int count = proxy_config_client_set_.size();
  base::RepeatingClosure closure =
      base::BarrierClosure(count, std::move(callback));
  for (auto& proxy_config_client : proxy_config_client_set_)
    proxy_config_client->FlushProxyConfig(closure);
}

}  // namespace bison
