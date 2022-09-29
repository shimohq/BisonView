// create by jiang947


#ifndef BISON_BROWSER_BISON_FEATURE_LIST_CREATOR_H_
#define BISON_BROWSER_BISON_FEATURE_LIST_CREATOR_H_


#include <memory>

#include "bison/browser/bv_browser_policy_connector.h"
#include "bison/browser/bv_field_trials.h"
#include "bison/browser/variations/bv_variations_service_client.h"

#include "base/metrics/field_trial.h"
#include "components/policy/core/browser/browser_policy_connector_base.h"
#include "components/prefs/pref_service.h"
#include "components/variations/service/variations_field_trial_creator.h"

namespace bison {

// Used by WebView to set up field trials based on the stored variations
// seed data. Once created this object must exist for the lifetime of the
// process as it contains the FieldTrialList that can be queried for the state
// of experiments.
class BvFeatureListCreator {
 public:
  BvFeatureListCreator();

  BvFeatureListCreator(const BvFeatureListCreator&) = delete;
  BvFeatureListCreator& operator=(const BvFeatureListCreator&) = delete;

  ~BvFeatureListCreator();

  // Initializes all necessary parameters to create the feature list and setup
  // field trials.
  void CreateFeatureListAndFieldTrials();

  void CreateLocalState();

  // Passes ownership of the |local_state_| to the caller.
  std::unique_ptr<PrefService> TakePrefService() {
    DCHECK(local_state_);
    return std::move(local_state_);
  }

  // Passes ownership of the |browser_policy_connector_| to the caller.
  std::unique_ptr<BvBrowserPolicyConnector> TakeBrowserPolicyConnector() {
    DCHECK(browser_policy_connector_);
    return std::move(browser_policy_connector_);
  }

 private:
  std::unique_ptr<PrefService> CreatePrefService();

  // Sets up the field trials and related initialization.
  void SetUpFieldTrials();

  // Stores the seed. VariationsSeedStore keeps a raw pointer to this, so it
  // must persist for the process lifetime. Not persisted accross runs.
  // If TakePrefService() is called, the caller will take the ownership
  // of this variable. Stop using this variable afterwards.
  std::unique_ptr<PrefService> local_state_;

  // Performs set up for any WebView specific field trials.
  std::unique_ptr<BvFieldTrials> bv_field_trials_;

  // Responsible for creating a feature list from the seed.
  std::unique_ptr<variations::VariationsFieldTrialCreator>
      variations_field_trial_creator_;

  std::unique_ptr<BvVariationsServiceClient> client_;

  std::unique_ptr<BvBrowserPolicyConnector> browser_policy_connector_;


};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_FEATURE_LIST_CREATOR_H_
