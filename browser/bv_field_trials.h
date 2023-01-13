// create by jiang947
#ifndef BISON_BROWSER_BISON_FIELD_TRIALS_H_
#define BISON_BROWSER_BISON_FIELD_TRIALS_H_

#include "components/variations/platform_field_trials.h"

// Responsible for setting up field trials specific to WebView. Currently all
// functions are stubs, as WebView has no specific field trials.
class BvFieldTrials : public variations::PlatformFieldTrials {
 public:
  BvFieldTrials() = default;

  BvFieldTrials(const BvFieldTrials&) = delete;
  BvFieldTrials& operator=(const BvFieldTrials&) = delete;

  ~BvFieldTrials() override = default;

  // variations::PlatformFieldTrials:
  void SetUpFieldTrials() override;
  void SetUpFeatureControllingFieldTrials(
      bool has_seed,
      const variations::EntropyProviders& entropy_providers,
      base::FeatureList* feature_list) override {}
};

#endif  // BISON_BROWSER_BISON_FIELD_TRIALS_H_
