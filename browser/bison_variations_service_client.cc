#include "bison/browser/bison_variations_service_client.h"

#include "base/bind.h"
#include "base/threading/scoped_blocking_call.h"
#include "build/build_config.h"
#include "components/version_info/android/channel_getter.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

using version_info::Channel;

namespace bison {
namespace {

// Gets the version number to use for variations seed simulation. Must be called
// on a thread where IO is allowed.
base::Version GetVersionForSimulation() {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);
  return version_info::GetVersion();
}

}  // namespace

BisonVariationsServiceClient::BisonVariationsServiceClient() {}

BisonVariationsServiceClient::~BisonVariationsServiceClient() {}

BisonVariationsServiceClient::VersionCallback 
BisonVariationsServiceClient::GetVersionForSimulationCallback() {
  return base::BindOnce(&GetVersionForSimulation);
}

scoped_refptr<network::SharedURLLoaderFactory>
BisonVariationsServiceClient::GetURLLoaderFactory() {
  return nullptr;
}

network_time::NetworkTimeTracker*
BisonVariationsServiceClient::GetNetworkTimeTracker() {
  return nullptr;
}

Channel BisonVariationsServiceClient::GetChannel() {
  return version_info::android::GetChannel();
}

bool BisonVariationsServiceClient::OverridesRestrictParameter(
    std::string* parameter) {
  return false;
}

bool BisonVariationsServiceClient::IsEnterprise() {
  return false;
}

}  // namespace bison
