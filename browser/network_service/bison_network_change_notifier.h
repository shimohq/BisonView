// create by jiang947


#ifndef BISON_BROWSER_NETWORK_SERVICE_BISON_NETWORK_CHANGE_NOTIFIER_H_
#define BISON_BROWSER_NETWORK_SERVICE_BISON_NETWORK_CHANGE_NOTIFIER_H_

#include "base/macros.h"
#include "net/android/network_change_notifier_delegate_android.h"
#include "net/base/network_change_notifier.h"

namespace bison {

// BisonNetworkChangeNotifier is similar to NetworkChangeNotifierAndroid except
// it only propagates max-bandwidth changes in order to make the Network
// Information API work in blink.
//
// This somewhat reduced functionality is necessary because of the way
// NetworkChangeNotifier is enabled in WebView. It is enabled only when there
// are living WebView instances (instead of using ApplicationStatus) hence the
// existing Chrome for Android implementation is not applicable as is
// (see crbug.com/529434).
class BisonNetworkChangeNotifier
    : public net::NetworkChangeNotifier,
      public net::NetworkChangeNotifierDelegateAndroid::Observer {
 public:
  ~BisonNetworkChangeNotifier() override;

  // NetworkChangeNotifier:
  ConnectionType GetCurrentConnectionType() const override;
  // Requires ACCESS_WIFI_STATE permission in order to provide precise WiFi link
  // speed.
  void GetCurrentMaxBandwidthAndConnectionType(
      double* max_bandwidth_mbps,
      ConnectionType* connection_type) const override;
  bool AreNetworkHandlesCurrentlySupported() const override;
  void GetCurrentConnectedNetworks(NetworkList* network_list) const override;
  ConnectionType GetCurrentNetworkConnectionType(
      NetworkHandle network) const override;
  NetworkHandle GetCurrentDefaultNetwork() const override;

  // NetworkChangeNotifierDelegateAndroid::Observer:
  void OnConnectionTypeChanged() override;
  void OnMaxBandwidthChanged(double max_bandwidth_mbps,
                             ConnectionType type) override;
  void OnNetworkConnected(NetworkHandle network) override;
  void OnNetworkSoonToDisconnect(NetworkHandle network) override;
  void OnNetworkDisconnected(NetworkHandle network) override;
  void OnNetworkMadeDefault(NetworkHandle network) override;

 private:
  friend class BvNetworkChangeNotifierFactory;

  BisonNetworkChangeNotifier(net::NetworkChangeNotifierDelegateAndroid* delegate);

  static NetworkChangeCalculatorParams DefaultNetworkChangeCalculatorParams();

  net::NetworkChangeNotifierDelegateAndroid* const delegate_;

  DISALLOW_COPY_AND_ASSIGN(BisonNetworkChangeNotifier);
};

}  // namespace bison

#endif  // BISON_BROWSER_NETWORK_SERVICE_BISON_NETWORK_CHANGE_NOTIFIER_H_
