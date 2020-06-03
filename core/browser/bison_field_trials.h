// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_BROWSER_BISON_FIELD_TRIALS_H_
#define BISON_CORE_BROWSER_BISON_FIELD_TRIALS_H_

#include "base/macros.h"
#include "components/variations/platform_field_trials.h"

// Responsible for setting up field trials specific to WebView. Currently all
// functions are stubs, as WebView has no specific field trials.
class BisonFieldTrials : public variations::PlatformFieldTrials {
 public:
  BisonFieldTrials() {}
  ~BisonFieldTrials() override {}

  // variations::PlatformFieldTrials:
  void SetupFieldTrials() override {}
  void SetupFeatureControllingFieldTrials(
      bool has_seed,
      base::FeatureList* feature_list) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(BisonFieldTrials);
};

#endif  // BISON_CORE_BROWSER_BISON_FIELD_TRIALS_H_
