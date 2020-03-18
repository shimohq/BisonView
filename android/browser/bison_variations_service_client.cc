// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/browser/bison_variations_service_client.h"

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

base::Callback<base::Version(void)>
BisonVariationsServiceClient::GetVersionForSimulationCallback() {
  return base::BindRepeating(&GetVersionForSimulation);
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
