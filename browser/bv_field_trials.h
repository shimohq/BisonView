// create by jiang947
#ifndef BISON_BROWSER_BISON_FIELD_TRIALS_H_
#define BISON_BROWSER_BISON_FIELD_TRIALS_H_

#include "base/macros.h"
#include "components/variations/platform_field_trials.h"

// Responsible for setting up field trials specific to WebView. Currently all
// functions are stubs, as WebView has no specific field trials.
class BvFieldTrials : public variations::PlatformFieldTrials {
 public:
  BvFieldTrials() {}
  ~BvFieldTrials() override {}

  // variations::PlatformFieldTrials:
  void SetupFieldTrials() override {}
  void SetupFeatureControllingFieldTrials(
      bool has_seed,
      base::FeatureList* feature_list) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(BvFieldTrials);
};

#endif  // BISON_BROWSER_BISON_FIELD_TRIALS_H_
