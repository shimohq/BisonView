#ifndef BISON_BROWSER_TRACING_AW_BACKGROUND_TRACING_METRICS_PROVIDER_H_
#define BISON_BROWSER_TRACING_AW_BACKGROUND_TRACING_METRICS_PROVIDER_H_

#include "components/tracing/common/background_tracing_metrics_provider.h"

namespace tracing {

class BvBackgroundTracingMetricsProvider
    : public BackgroundTracingMetricsProvider {
 public:
  BvBackgroundTracingMetricsProvider();

  BvBackgroundTracingMetricsProvider(
      const BvBackgroundTracingMetricsProvider&) = delete;
  BvBackgroundTracingMetricsProvider& operator=(
      const BvBackgroundTracingMetricsProvider&) = delete;

  ~BvBackgroundTracingMetricsProvider() override;

  // metrics::MetricsProvider:
  void Init() override;

 private:
  // BackgroundTracingMetricsProvider:
  void ProvideEmbedderMetrics(
      metrics::ChromeUserMetricsExtension* uma_proto,
      base::HistogramSnapshotManager* snapshot_manager) override;
};

}  // namespace tracing

#endif  // BISON_BROWSER_TRACING_AW_BACKGROUND_TRACING_METRICS_PROVIDER_H_
