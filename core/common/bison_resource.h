// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_COMMON_BISON_RESOURCE_H_
#define BISON_CORE_COMMON_BISON_RESOURCE_H_

#include <string>
#include <vector>

// bison implements these with a JNI call to the
// BisonResource Java class.
namespace bison {
namespace BisonResource {

std::vector<std::string> GetConfigKeySystemUuidMapping();

}  // namespace BisonResource
}  // namsespace bison

#endif  // BISON_CORE_COMMON_BISON_RESOURCE_H_
