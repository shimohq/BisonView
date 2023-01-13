#include "bison/browser/tracing/bv_tracing_delegate.h"

#include <memory>

#include "base/notreached.h"
#include "base/values.h"
#include "components/version_info/version_info.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace bison {

BvTracingDelegate::BvTracingDelegate() {}
BvTracingDelegate::~BvTracingDelegate() {}

bool BvTracingDelegate::IsAllowedToBeginBackgroundScenario(
    const content::BackgroundTracingConfig& config,
    bool requires_anonymized_data) {
  // Background tracing is allowed in general and can be restricted when
  // configuring BackgroundTracingManager.
  return true;
}

bool BvTracingDelegate::IsAllowedToEndBackgroundScenario(
    const content::BackgroundTracingConfig& config,
    bool requires_anonymized_data,
  bool is_crash_scenario) {
  // Background tracing is allowed in general and can be restricted when
  // configuring BackgroundTracingManager.
  return true;
}

absl::optional<base::Value::Dict> BvTracingDelegate::GenerateMetadataDict() {
  base::Value::Dict metadata_dict;
  metadata_dict.Set("revision", version_info::GetLastChange());
  return metadata_dict;
}

}  // namespace bison
