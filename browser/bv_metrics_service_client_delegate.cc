
#include "bison/browser/bv_metrics_service_client_delegate.h"

#include "bison/browser/bv_browser_process.h"
#include "bison/browser/lifecycle/bv_contents_lifecycle_notifier.h"
#include "bison/browser/metrics/bv_metrics_service_client.h"
#include "bison/browser/metrics/renderer_process_metrics_provider.h"
#include "bison/browser/metrics/visibility_metrics_provider.h"
#include "bison/browser/page_load_metrics/bv_page_load_metrics_provider.h"
#include "bison/browser/tracing/bv_background_tracing_metrics_provider.h"

#include "components/metrics/component_metrics_provider.h"
#include "components/metrics/metrics_service.h"

namespace bison {

BvMetricsServiceClientDelegate::BvMetricsServiceClientDelegate() = default;
BvMetricsServiceClientDelegate::~BvMetricsServiceClientDelegate() = default;

void BvMetricsServiceClientDelegate::RegisterAdditionalMetricsProviders(
    metrics::MetricsService* service) {
  service->RegisterMetricsProvider(
      std::make_unique<BvPageLoadMetricsProvider>());
  service->RegisterMetricsProvider(std::make_unique<VisibilityMetricsProvider>(
      BvBrowserProcess::GetInstance()->visibility_metrics_logger()));
  service->RegisterMetricsProvider(
      std::make_unique<RendererProcessMetricsProvider>());
  // service->RegisterMetricsProvider(
  //     std::make_unique<metrics::ComponentMetricsProvider>(
  //         std::make_unique<BvComponentMetricsProviderDelegate>(
  //             BvMetricsServiceClient::GetInstance())));
  service->RegisterMetricsProvider(
      std::make_unique<tracing::BvBackgroundTracingMetricsProvider>());
}

void BvMetricsServiceClientDelegate::AddWebViewAppStateObserver(
    WebViewAppStateObserver* observer) {
  BvContentsLifecycleNotifier::GetInstance().AddObserver(observer);
}

bool BvMetricsServiceClientDelegate::HasBvContentsEverCreated() const {
  return BvContentsLifecycleNotifier::GetInstance()
      .has_bv_contents_ever_created();
}

}  // namespace bison
