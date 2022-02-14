
#include "bison/browser/network_service/bv_network_change_notifier_factory.h"

#include "bison/browser/network_service/bison_network_change_notifier.h"
#include "base/memory/ptr_util.h"

namespace bison {

BvNetworkChangeNotifierFactory::BvNetworkChangeNotifierFactory() {}

BvNetworkChangeNotifierFactory::~BvNetworkChangeNotifierFactory() {}

std::unique_ptr<net::NetworkChangeNotifier>
BvNetworkChangeNotifierFactory::CreateInstance() {
  return base::WrapUnique(new BisonNetworkChangeNotifier(&delegate_));
}

}  // namespace bison
