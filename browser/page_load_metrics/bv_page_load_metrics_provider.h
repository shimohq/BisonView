#ifndef BISON_BROWSER_PAGE_LOAD_METRICS_AW_PAGE_LOAD_METRICS_PROVIDER_H_
#define BISON_BROWSER_PAGE_LOAD_METRICS_AW_PAGE_LOAD_METRICS_PROVIDER_H_

#include "components/metrics/metrics_provider.h"

namespace bison {

// MetricsProvider that interfaces with page_load_metrics.
class BvPageLoadMetricsProvider : public metrics::MetricsProvider {
 public:
  BvPageLoadMetricsProvider();

  BvPageLoadMetricsProvider(const BvPageLoadMetricsProvider&) = delete;
  BvPageLoadMetricsProvider& operator=(const BvPageLoadMetricsProvider&) =
      delete;

  ~BvPageLoadMetricsProvider() override;

  // metrics:MetricsProvider:
  void OnAppEnterBackground() override;
};

}  // namespace bison

#endif  // BISON_BROWSER_PAGE_LOAD_METRICS_AW_PAGE_LOAD_METRICS_PROVIDER_H_
