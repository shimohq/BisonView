#ifndef BISON_BROWSER_METRICS_RENDERER_PROCESS_METRICS_PROVIDER_H_
#define BISON_BROWSER_METRICS_RENDERER_PROCESS_METRICS_PROVIDER_H_

#include "components/metrics/metrics_provider.h"

namespace bison {

// RendererProcessMetricsProvider is responsible for logging whether a WebView
// instance is running in single process (with an in process renderer) or multi
// process (with an out of process renderer) mode.
class RendererProcessMetricsProvider : public metrics::MetricsProvider {
 public:
  RendererProcessMetricsProvider() = default;

  ~RendererProcessMetricsProvider() override = default;

  void ProvideCurrentSessionData(
      metrics::ChromeUserMetricsExtension* uma_proto) override;

  RendererProcessMetricsProvider(const RendererProcessMetricsProvider&) =
      delete;

  RendererProcessMetricsProvider& operator=(
      const RendererProcessMetricsProvider&) = delete;
};

}  // namespace bison

#endif  // BISON_BROWSER_METRICS_RENDERER_PROCESS_METRICS_PROVIDER_H_
