#ifndef BISON_BROWSER_TRACING_AW_TRACING_DELEGATE_H_
#define BISON_BROWSER_TRACING_AW_TRACING_DELEGATE_H_

#include <memory>

#include "content/public/browser/tracing_delegate.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class Value;
}  // namespace base

namespace bison {

class BvTracingDelegate : public content::TracingDelegate {
 public:
  BvTracingDelegate();
  ~BvTracingDelegate() override;

  // content::TracingDelegate implementation:
  bool IsAllowedToBeginBackgroundScenario(
      const content::BackgroundTracingConfig& config,
      bool requires_anonymized_data) override;
  bool IsAllowedToEndBackgroundScenario(
      const content::BackgroundTracingConfig& config,
      bool requires_anonymized_data,
      bool is_crash_scenario) override;
  absl::optional<base::Value> GenerateMetadataDict() override;
};

}  // namespace bison

#endif  // BISON_BROWSER_TRACING_AW_TRACING_DELEGATE_H_
