
#include "bison/browser/bv_field_trials.h"

#include "base/base_paths_android.h"
#include "base/feature_list.h"
#include "base/metrics/persistent_histogram_allocator.h"
#include "base/path_service.h"
#include "components/metrics/persistent_histograms.h"

void BvFieldTrials::SetUpFieldTrials() {
  // Persistent histograms must be enabled as soon as possible.
  base::FilePath metrics_dir;
  if (base::PathService::Get(base::DIR_ANDROID_APP_DATA, &metrics_dir)) {
    InstantiatePersistentHistograms(metrics_dir);
  }
}
