// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_BROWSER_SCOPED_ADD_FEATURE_FLAGS_H_
#define BISON_CORE_BROWSER_SCOPED_ADD_FEATURE_FLAGS_H_

#include <string>
#include <vector>

#include "base/feature_list.h"

namespace base {
class CommandLine;
}

namespace bison {

class ScopedAddFeatureFlags {
 public:
  explicit ScopedAddFeatureFlags(base::CommandLine* cl);
  ~ScopedAddFeatureFlags();

  // Any existing (user set) enable/disable takes precedence.
  void EnableIfNotSet(const base::Feature& feature);
  void DisableIfNotSet(const base::Feature& feature);

 private:
  void AddFeatureIfNotSet(const base::Feature& feature, bool enable);

  base::CommandLine* const cl_;
  std::vector<std::string> enabled_features_;
  std::vector<std::string> disabled_features_;

  DISALLOW_COPY_AND_ASSIGN(ScopedAddFeatureFlags);
};

}  // namespace bison

#endif  // BISON_CORE_BROWSER_SCOPED_ADD_FEATURE_FLAGS_H_
