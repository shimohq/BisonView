// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/core/browser/network_service/bison_network_change_notifier_factory.h"

#include "bison/core/browser/network_service/bison_network_change_notifier.h"
#include "base/memory/ptr_util.h"

namespace bison {

BisonNetworkChangeNotifierFactory::BisonNetworkChangeNotifierFactory() {}

BisonNetworkChangeNotifierFactory::~BisonNetworkChangeNotifierFactory() {}

std::unique_ptr<net::NetworkChangeNotifier>
BisonNetworkChangeNotifierFactory::CreateInstance() {
  return base::WrapUnique(new BisonNetworkChangeNotifier(&delegate_));
}

}  // namespace bison
