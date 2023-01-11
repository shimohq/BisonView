#ifndef BISON_BROWSER_METRICS_VISIBILITY_METRICS_PROVIDER_H_
#define BISON_BROWSER_METRICS_VISIBILITY_METRICS_PROVIDER_H_

#include "base/memory/raw_ptr.h"
#include "components/metrics/metrics_provider.h"

namespace bison {

class VisibilityMetricsLogger;

class VisibilityMetricsProvider : public metrics::MetricsProvider {
 public:
  explicit VisibilityMetricsProvider(VisibilityMetricsLogger* logger);
  ~VisibilityMetricsProvider() override;

  VisibilityMetricsProvider() = delete;
  VisibilityMetricsProvider(const VisibilityMetricsProvider&) = delete;

  // metrics::MetricsProvider
  void ProvideCurrentSessionData(
      metrics::ChromeUserMetricsExtension* uma_proto) override;

 private:
  raw_ptr<VisibilityMetricsLogger> logger_;
};

}  // namespace bison

#endif  // BISON_BROWSER_METRICS_VISIBILITY_METRICS_PROVIDER_H_
