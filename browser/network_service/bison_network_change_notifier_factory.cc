
#include "bison/browser/network_service/bison_network_change_notifier_factory.h"

#include "bison/browser/network_service/bison_network_change_notifier.h"
#include "base/memory/ptr_util.h"

namespace bison {

BisonNetworkChangeNotifierFactory::BisonNetworkChangeNotifierFactory() {}

BisonNetworkChangeNotifierFactory::~BisonNetworkChangeNotifierFactory() {}

std::unique_ptr<net::NetworkChangeNotifier>
BisonNetworkChangeNotifierFactory::CreateInstance() {
  return base::WrapUnique(new BisonNetworkChangeNotifier(&delegate_));
}

}  // namespace bison
