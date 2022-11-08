#ifndef BISON_BROWSER_METRICS_AW_COMPONENT_METRICS_PROVIDER_DELEGATE_H_
#define BISON_BROWSER_METRICS_AW_COMPONENT_METRICS_PROVIDER_DELEGATE_H_

#include "base/memory/raw_ptr.h"
#include "components/metrics/component_metrics_provider.h"

namespace component_updater {
struct ComponentInfo;
}  // namespace component_updater

namespace bison {

class BvMetricsServiceClient;

// WebView delegate to provide WebView's own list of loaded components to be
// recorded in the system profile UMA log. Unlike chrome, WebView doesn't use
// `component_updater::ComponentUpdateService` to load or keep track of
// components.
class BvComponentMetricsProviderDelegate
    : public metrics::ComponentMetricsProviderDelegate {
 public:
  explicit BvComponentMetricsProviderDelegate(BvMetricsServiceClient* client);
  ~BvComponentMetricsProviderDelegate() override = default;

  // ComponentsInfoProvider:
  std::vector<component_updater::ComponentInfo> GetComponents() override;

 private:
  raw_ptr<BvMetricsServiceClient> client_;
};

}  // namespace bison

#endif  // BISON_BROWSER_METRICS_AW_COMPONENT_METRICS_PROVIDER_DELEGATE_H_
