#include "bison/browser/tracing/bv_tracing_delegate.h"

#include <memory>

#include "bison/browser/bv_browser_process.h"

#include "base/notreached.h"
#include "base/values.h"
#include "components/tracing/common/background_tracing_state_manager.h"
#include "components/tracing/common/background_tracing_utils.h"
#include "components/tracing/common/pref_names.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/background_tracing_config.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace bison {

bool IsBackgroundTracingCommandLine() {
  auto tracing_mode = tracing::GetBackgroundTracingSetupMode();
  if (tracing_mode == tracing::BackgroundTracingSetupMode::kFromConfigFile ||
      tracing_mode ==
          tracing::BackgroundTracingSetupMode::kFromFieldTrialLocalOutput) {
    return true;
  }
  return false;
}

BvTracingDelegate::BvTracingDelegate() {}
BvTracingDelegate::~BvTracingDelegate() {}
// static
void BvTracingDelegate::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(tracing::kBackgroundTracingSessionState);
}

bool BvTracingDelegate::IsAllowedToBeginBackgroundScenario(
    const content::BackgroundTracingConfig& config,
    bool requires_anonymized_data) {
  // Background tracing is allowed in general and can be restricted when
  // configuring BackgroundTracingManager.
  if (IsBackgroundTracingCommandLine())
    return true;

  tracing::BackgroundTracingStateManager& state =
      tracing::BackgroundTracingStateManager::GetInstance();
  state.Initialize(BvBrowserProcess::GetInstance()->local_state());

  // Don't start a new trace if the previous trace did not end.
  if (state.DidLastSessionEndUnexpectedly()) {
    tracing::RecordDisallowedMetric(
        tracing::TracingFinalizationDisallowedReason::
            kLastTracingSessionDidNotEnd);
    return false;
  }

  // TODO(crbug.com/1290887): check the trace limit per week (to be implemented
  // later)

  state.NotifyTracingStarted();
  return true;
}

bool BvTracingDelegate::IsAllowedToEndBackgroundScenario(
    const content::BackgroundTracingConfig& config,
    bool requires_anonymized_data,
  bool is_crash_scenario) {
  // Background tracing is allowed in general and can be restricted when
  // configuring BackgroundTracingManager.
  if (IsBackgroundTracingCommandLine())
    return true;

  tracing::BackgroundTracingStateManager& state =
      tracing::BackgroundTracingStateManager::GetInstance();
  state.NotifyFinalizationStarted();

  // TODO(crbug.com/1290887): check the trace limit per week (to be implemented
  // later)

  state.OnScenarioUploaded(config.scenario_name());
  return true;
}

absl::optional<base::Value::Dict> BvTracingDelegate::GenerateMetadataDict() {
  base::Value::Dict metadata_dict;
  metadata_dict.Set("revision", version_info::GetLastChange());
  return metadata_dict;
}

}  // namespace bison
