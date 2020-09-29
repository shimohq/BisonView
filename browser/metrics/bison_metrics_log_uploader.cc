#include "bison/browser/metrics/bison_metrics_log_uploader.h"

#include "base/android/jni_array.h"
#include "components/metrics/log_decoder.h"

using base::android::ScopedJavaLocalRef;
using base::android::ToJavaByteArray;

namespace bison {

BisonMetricsLogUploader::BisonMetricsLogUploader(
    const metrics::MetricsLogUploader::UploadCallback& on_upload_complete)
    : on_upload_complete_(on_upload_complete) {}

BisonMetricsLogUploader::~BisonMetricsLogUploader() {}

void BisonMetricsLogUploader::UploadLog(
    const std::string& compressed_log_data,
    const std::string& /*log_hash*/,
    const std::string& /*log_signature*/,
    const metrics::ReportingInfo& reporting_info) {
  on_upload_complete_.Run(200, 0, true);
}

}  // namespace bison
