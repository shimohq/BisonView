// create by jiang947 


#ifndef BISON_BROWSER_NETWORK_SERVICE_BISON_NETWORK_CHANGE_NOTIFIER_FACTORY_H_
#define BISON_BROWSER_NETWORK_SERVICE_BISON_NETWORK_CHANGE_NOTIFIER_FACTORY_H_

#include <memory>

#include "net/android/network_change_notifier_delegate_android.h"
#include "net/base/network_change_notifier_factory.h"

namespace net {
class NetworkChangeNotifier;
}

namespace bison {

// BisonNetworkChangeNotifierFactory creates WebView-specific specialization of
// NetworkChangeNotifier. See aw_network_change_notifier.h for more details.
class BisonNetworkChangeNotifierFactory :
    public net::NetworkChangeNotifierFactory {
 public:
  // Must be called on the JNI thread.
  BisonNetworkChangeNotifierFactory();

  // Must be called on the JNI thread.
  ~BisonNetworkChangeNotifierFactory() override;

  // NetworkChangeNotifierFactory:
  std::unique_ptr<net::NetworkChangeNotifier> CreateInstance() override;

 private:
  // Delegate passed to the instances created by this class.
  net::NetworkChangeNotifierDelegateAndroid delegate_;

  DISALLOW_COPY_AND_ASSIGN(BisonNetworkChangeNotifierFactory);
};

}  // namespace bison

#endif  // BISON_BROWSER_NETWORK_SERVICE_BISON_NETWORK_CHANGE_NOTIFIER_FACTORY_H_
