// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/core/browser/network_service/bison_network_change_notifier.h"

namespace bison {

BisonNetworkChangeNotifier::~BisonNetworkChangeNotifier() {
  delegate_->RemoveObserver(this);
}

net::NetworkChangeNotifier::ConnectionType
BisonNetworkChangeNotifier::GetCurrentConnectionType() const {
  return delegate_->GetCurrentConnectionType();
}

void BisonNetworkChangeNotifier::GetCurrentMaxBandwidthAndConnectionType(
    double* max_bandwidth_mbps,
    ConnectionType* connection_type) const {
  delegate_->GetCurrentMaxBandwidthAndConnectionType(max_bandwidth_mbps,
                                                     connection_type);
}

bool BisonNetworkChangeNotifier::AreNetworkHandlesCurrentlySupported() const {
  return false;
}

void BisonNetworkChangeNotifier::GetCurrentConnectedNetworks(
    NetworkChangeNotifier::NetworkList* networks) const {
  delegate_->GetCurrentlyConnectedNetworks(networks);
}

net::NetworkChangeNotifier::ConnectionType
BisonNetworkChangeNotifier::GetCurrentNetworkConnectionType(
    NetworkHandle network) const {
  return delegate_->GetNetworkConnectionType(network);
}

net::NetworkChangeNotifier::NetworkHandle
BisonNetworkChangeNotifier::GetCurrentDefaultNetwork() const {
  return delegate_->GetCurrentDefaultNetwork();
}

void BisonNetworkChangeNotifier::OnConnectionTypeChanged() {}

void BisonNetworkChangeNotifier::OnMaxBandwidthChanged(
    double max_bandwidth_mbps,
    ConnectionType type) {
  // Note that this callback is sufficient for Network Information API because
  // it also occurs on type changes (see network_change_notifier.h).
  NetworkChangeNotifier::NotifyObserversOfMaxBandwidthChange(max_bandwidth_mbps,
                                                             type);
}

void BisonNetworkChangeNotifier::OnNetworkConnected(NetworkHandle network) {}
void BisonNetworkChangeNotifier::OnNetworkSoonToDisconnect(
    NetworkHandle network) {}
void BisonNetworkChangeNotifier::OnNetworkDisconnected(
    NetworkHandle network) {}
void BisonNetworkChangeNotifier::OnNetworkMadeDefault(NetworkHandle network){}

BisonNetworkChangeNotifier::BisonNetworkChangeNotifier(
    net::NetworkChangeNotifierDelegateAndroid* delegate)
    : net::NetworkChangeNotifier(DefaultNetworkChangeCalculatorParams()),
      delegate_(delegate) {
  delegate_->AddObserver(this);
}

// static
net::NetworkChangeNotifier::NetworkChangeCalculatorParams
BisonNetworkChangeNotifier::DefaultNetworkChangeCalculatorParams() {
  net::NetworkChangeNotifier::NetworkChangeCalculatorParams params;
  // Use defaults as in network_change_notifier_android.cc
  params.ip_address_offline_delay_ = base::TimeDelta::FromSeconds(1);
  params.ip_address_online_delay_ = base::TimeDelta::FromSeconds(1);
  params.connection_type_offline_delay_ = base::TimeDelta::FromSeconds(0);
  params.connection_type_online_delay_ = base::TimeDelta::FromSeconds(0);
  return params;
}

}  // namespace bison
