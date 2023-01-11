

#include "bison/browser/page_load_metrics/bv_page_load_metrics_memory_tracker_factory.h"

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/page_load_metrics/browser/page_load_metrics_memory_tracker.h"

namespace bison {

page_load_metrics::PageLoadMetricsMemoryTracker*
BvPageLoadMetricsMemoryTrackerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<page_load_metrics::PageLoadMetricsMemoryTracker*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

BvPageLoadMetricsMemoryTrackerFactory*
BvPageLoadMetricsMemoryTrackerFactory::GetInstance() {
  return base::Singleton<BvPageLoadMetricsMemoryTrackerFactory>::get();
}

BvPageLoadMetricsMemoryTrackerFactory::BvPageLoadMetricsMemoryTrackerFactory()
    : BrowserContextKeyedServiceFactory(
          "PageLoadMetricsMemoryTracker",
          BrowserContextDependencyManager::GetInstance()) {}

bool BvPageLoadMetricsMemoryTrackerFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return base::FeatureList::IsEnabled(features::kV8PerFrameMemoryMonitoring);
}

KeyedService* BvPageLoadMetricsMemoryTrackerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new page_load_metrics::PageLoadMetricsMemoryTracker();
}

content::BrowserContext*
BvPageLoadMetricsMemoryTrackerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return context;
}

}  // namespace bison
