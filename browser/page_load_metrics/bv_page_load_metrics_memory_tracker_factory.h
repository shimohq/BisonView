#ifndef BISON_BROWSER_PAGE_LOAD_METRICS_BV_PAGE_LOAD_METRICS_MEMORY_TRACKER_FACTORY_H_
#define BISON_BROWSER_PAGE_LOAD_METRICS_BV_PAGE_LOAD_METRICS_MEMORY_TRACKER_FACTORY_H_

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace page_load_metrics {
class PageLoadMetricsMemoryTracker;
}  // namespace page_load_metrics

namespace bison {

class BvPageLoadMetricsMemoryTrackerFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static page_load_metrics::PageLoadMetricsMemoryTracker* GetForBrowserContext(
      content::BrowserContext* context);

  static BvPageLoadMetricsMemoryTrackerFactory* GetInstance();

  BvPageLoadMetricsMemoryTrackerFactory();

 private:
  // BrowserContextKeyedServiceFactory:
  bool ServiceIsCreatedWithBrowserContext() const override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace bison

#endif  // BISON_BROWSER_PAGE_LOAD_METRICS_BV_PAGE_LOAD_METRICS_MEMORY_TRACKER_FACTORY_H_
