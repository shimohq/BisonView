#include "bison/browser/variations/bv_variations_service_client.h"

#include "components/version_info/android/channel_getter.h"
#include "components/version_info/version_info.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

using version_info::Channel;

namespace bison {

BvVariationsServiceClient::BvVariationsServiceClient() = default;

BvVariationsServiceClient::~BvVariationsServiceClient() = default;

base::Version BvVariationsServiceClient::GetVersionForSimulation() {
  return version_info::GetVersion();
}

scoped_refptr<network::SharedURLLoaderFactory>
BvVariationsServiceClient::GetURLLoaderFactory() {
  return nullptr;
}

network_time::NetworkTimeTracker*
BvVariationsServiceClient::GetNetworkTimeTracker() {
  return nullptr;
}

Channel BvVariationsServiceClient::GetChannel() {
  return version_info::android::GetChannel();
}

bool BvVariationsServiceClient::OverridesRestrictParameter(
    std::string* parameter) {
  return false;
}

bool BvVariationsServiceClient::IsEnterprise() {
  return false;
}

}  // namespace bison
