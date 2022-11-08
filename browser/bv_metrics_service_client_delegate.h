#ifndef BISON_BROWSER_BV_METRICS_SERVICE_CLIENT_DELEGATE_H_
#define BISON_BROWSER_BV_METRICS_SERVICE_CLIENT_DELEGATE_H_

#include "bison/browser/metrics/bv_metrics_service_client.h"

namespace bison {

// Interceptor to handle urls for media assets in the apk.
class BvMetricsServiceClientDelegate : public BvMetricsServiceClient::Delegate {
 public:
  BvMetricsServiceClientDelegate();
  ~BvMetricsServiceClientDelegate() override;

  // AwMetricsServiceClient::Delegate
  void RegisterAdditionalMetricsProviders(
      metrics::MetricsService* service) override;
  void AddWebViewAppStateObserver(WebViewAppStateObserver* observer) override;
  bool HasBvContentsEverCreated() const override;
};

}  // namespace bison

#endif  // BISON_BROWSER_BV_METRICS_SERVICE_CLIENT_DELEGATE_H_
