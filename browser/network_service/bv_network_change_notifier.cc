#include "bison/browser/network_service/bv_network_change_notifier.h"

namespace bison {

BvNetworkChangeNotifier::~BvNetworkChangeNotifier() {
  delegate_->UnregisterObserver(this);
}

net::NetworkChangeNotifier::ConnectionType
BvNetworkChangeNotifier::GetCurrentConnectionType() const {
  return delegate_->GetCurrentConnectionType();
}

void BvNetworkChangeNotifier::GetCurrentMaxBandwidthAndConnectionType(
    double* max_bandwidth_mbps,
    ConnectionType* connection_type) const {
  delegate_->GetCurrentMaxBandwidthAndConnectionType(max_bandwidth_mbps,
                                                     connection_type);
}

bool BvNetworkChangeNotifier::AreNetworkHandlesCurrentlySupported() const {
  return false;
}

void BvNetworkChangeNotifier::GetCurrentConnectedNetworks(
    NetworkChangeNotifier::NetworkList* networks) const {
  delegate_->GetCurrentlyConnectedNetworks(networks);
}

net::NetworkChangeNotifier::ConnectionType
BvNetworkChangeNotifier::GetCurrentNetworkConnectionType(
    NetworkHandle network) const {
  return delegate_->GetNetworkConnectionType(network);
}

net::NetworkChangeNotifier::NetworkHandle
BvNetworkChangeNotifier::GetCurrentDefaultNetwork() const {
  return delegate_->GetCurrentDefaultNetwork();
}

void BvNetworkChangeNotifier::OnConnectionTypeChanged() {}

void BvNetworkChangeNotifier::OnConnectionCostChanged() {}

void BvNetworkChangeNotifier::OnMaxBandwidthChanged(
    double max_bandwidth_mbps,
    ConnectionType type) {
  // Note that this callback is sufficient for Network Information API because
  // it also occurs on type changes (see network_change_notifier.h).
  NetworkChangeNotifier::NotifyObserversOfMaxBandwidthChange(max_bandwidth_mbps,
                                                             type);
}

void BvNetworkChangeNotifier::OnNetworkConnected(NetworkHandle network) {}
void BvNetworkChangeNotifier::OnNetworkSoonToDisconnect(
    NetworkHandle network) {}
void BvNetworkChangeNotifier::OnNetworkDisconnected(
    NetworkHandle network) {}
void BvNetworkChangeNotifier::OnNetworkMadeDefault(NetworkHandle network){}
void BvNetworkChangeNotifier::OnDefaultNetworkActive() {}

BvNetworkChangeNotifier::BvNetworkChangeNotifier(
    net::NetworkChangeNotifierDelegateAndroid* delegate)
    : net::NetworkChangeNotifier(DefaultNetworkChangeCalculatorParams()),
      delegate_(delegate) {
  delegate_->RegisterObserver(this);
}

// static
net::NetworkChangeNotifier::NetworkChangeCalculatorParams
BvNetworkChangeNotifier::DefaultNetworkChangeCalculatorParams() {
  net::NetworkChangeNotifier::NetworkChangeCalculatorParams params;
  // Use defaults as in network_change_notifier_android.cc
  params.ip_address_offline_delay_ = base::Seconds(1);
  params.ip_address_online_delay_ = base::Seconds(1);
  params.connection_type_offline_delay_ = base::Seconds(0);
  params.connection_type_online_delay_ = base::Seconds(0);
  return params;
}

}  // namespace bison
