
#include "bison/browser/metrics/visibility_metrics_provider.h"

#include "bison/browser/metrics/visibility_metrics_logger.h"

namespace bison {

VisibilityMetricsProvider::VisibilityMetricsProvider(
    VisibilityMetricsLogger* logger)
    : logger_(logger) {}

VisibilityMetricsProvider::~VisibilityMetricsProvider() = default;

void VisibilityMetricsProvider::ProvideCurrentSessionData(
    metrics::ChromeUserMetricsExtension* uma_proto) {
  logger_->RecordMetrics();
}

}  // namespace bison
