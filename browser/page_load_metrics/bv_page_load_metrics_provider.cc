#include "bison/browser/page_load_metrics/bv_page_load_metrics_provider.h"

#include "bison/browser/bv_contents.h"
#include "bison/browser/lifecycle/bv_contents_lifecycle_notifier.h"
#include "components/page_load_metrics/browser/metrics_web_contents_observer.h"

namespace bison {

BvPageLoadMetricsProvider::BvPageLoadMetricsProvider() = default;

BvPageLoadMetricsProvider::~BvPageLoadMetricsProvider() = default;

void BvPageLoadMetricsProvider::OnAppEnterBackground() {
  std::vector<const BvContents*> all_bv_contents(
      BvContentsLifecycleNotifier::GetInstance().GetAllBvContents());
  for (auto* bv_contents : all_bv_contents) {
    page_load_metrics::MetricsWebContentsObserver* observer =
        page_load_metrics::MetricsWebContentsObserver::FromWebContents(
            bv_contents->web_contents());
    if (observer)
      observer->FlushMetricsOnAppEnterBackground();
  }
}

}  // namespace bison
