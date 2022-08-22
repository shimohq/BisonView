// create by jiang947


#ifndef BISON_BROWSER_METRICS_BISON_METRICS_LOG_UPLOADER_H_
#define BISON_BROWSER_METRICS_BISON_METRICS_LOG_UPLOADER_H_

#include <jni.h>
#include <string>

#include "components/metrics/metrics_log_uploader.h"

namespace bison {

// Uploads UMA logs for WebView using the platform logging mechanism.
class BisonMetricsLogUploader : public ::metrics::MetricsLogUploader {
 public:
  explicit BisonMetricsLogUploader(
      const ::metrics::MetricsLogUploader::UploadCallback& on_upload_complete);

  ~BisonMetricsLogUploader() override;

  // ::metrics::MetricsLogUploader:
  // Note: |log_hash| and |log_signature| are only used by the normal UMA
  // server. WebView uses the platform logging mechanism instead of the normal
  // UMA server, so |log_hash| and |log_signature| aren't used.
  void UploadLog(const std::string& compressed_log_data,
                 const std::string& log_hash,
                 const std::string& log_signature,
                 const metrics::ReportingInfo& reporting_info) override;

 private:
  const metrics::MetricsLogUploader::UploadCallback on_upload_complete_;


};

}  // namespace bison

#endif  // BISON_BROWSER_METRICS_BISON_METRICS_LOG_UPLOADER_H_
