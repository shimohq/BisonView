// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_RENDERER_BISON_KEY_SYSTEMS_H_
#define BISON_ANDROID_RENDERER_BISON_KEY_SYSTEMS_H_

#include <memory>
#include <vector>

#include "media/base/key_system_properties.h"

namespace bison {

void BisonAddKeySystems(std::vector<std::unique_ptr<media::KeySystemProperties>>*
                         key_systems_properties);

}  // namespace bison

#endif  // BISON_ANDROID_RENDERER_BISON_KEY_SYSTEMS_H_
