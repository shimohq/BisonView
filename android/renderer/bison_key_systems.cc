// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/renderer/bison_key_systems.h"
#include "components/cdm/renderer/android_key_systems.h"

namespace bison {

void BisonAddKeySystems(std::vector<std::unique_ptr<media::KeySystemProperties>>*
                         key_systems_properties) {
  cdm::AddAndroidWidevine(key_systems_properties);
  cdm::AddAndroidPlatformKeySystems(key_systems_properties);
}

}  // namespace bison
