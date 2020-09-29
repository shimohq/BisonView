// create by jiang947 

#ifndef BISON_BROWSER_METRICS_BISON_METRICS_SERVICE_CLIENT_H_
#define BISON_BROWSER_METRICS_BISON_METRICS_SERVICE_CLIENT_H_

#include <memory>
#include <string>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/sequence_checker.h"
#include "components/metrics/enabled_state_provider.h"
#include "components/metrics/metrics_log_uploader.h"
#include "components/metrics/metrics_service_client.h"

class PrefService;

namespace metrics {
class MetricsStateManager;
}

namespace bison {

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
// TODO(https://crbug.com/1012025): remove this when the kInstallDate pref has
// been persisted for one or two milestones. Visible for testing.
enum class BackfillInstallDate {
  kValidInstallDatePref = 0,
  kCouldNotGetPackageManagerInstallDate = 1,
  kPersistedPackageManagerInstallDate = 2,
  kMaxValue = kPersistedPackageManagerInstallDate,
};

// BisonMetricsServiceClient is a singleton which manages WebView metrics
// collection.
//
// Metrics should be enabled iff all these conditions are met:
//  - The user has not opted out (controlled by GMS).
//  - The app has not opted out (controlled by manifest tag).
//  - This client is in the 2% sample (controlled by client ID hash).
// The first two are recorded in |user_consent_| and |app_consent_|, which are
// set by SetHaveMetricsConsent(). The last is recorded in |is_in_sample_|.
//
// Metrics are pseudonymously identified by a randomly-generated "client ID".
// WebView stores this in prefs, written to the app's data directory. There's a
// different such directory for each user, for each app, on each device. So the
// ID should be unique per (device, app, user) tuple.
//
// To avoid the appearance that we're doing anything sneaky, the client ID
// should only be created and retained when neither the user nor the app have
// opted out. Otherwise, the presence of the ID could give the impression that
// metrics were being collected.
//
// WebView metrics set up happens like so:
//
//   startup
//      │
//      ├────────────┐
//      │            ▼
//      │         query GMS for consent
//      ▼            │
//   Initialize()    │
//      │            ▼
//      │         SetHaveMetricsConsent()
//      │            │
//      │ ┌──────────┘
//      ▼ ▼
//   MaybeStartMetrics()
//      │
//      ▼
//   MetricsService::Start()
//
// All the named functions in this diagram happen on the UI thread. Querying GMS
// happens in the background, and the result is posted back to the UI thread, to
// SetHaveMetricsConsent(). Querying GMS is slow, so SetHaveMetricsConsent()
// typically happens after Initialize(), but it may happen before.
//
// Each path sets a flag, |init_finished_| or |set_consent_finished_|, to show
// that path has finished, and then calls MaybeStartMetrics(). When
// MaybeStartMetrics() is called the first time, it sees only one flag is true,
// and does nothing. When MaybeStartMetrics() is called the second time, it
// decides whether to start metrics.
//
// If consent was granted, MaybeStartMetrics() determines sampling by hashing
// the client ID (generating a new ID if there was none). If this client is in
// the sample, it then calls MetricsService::Start(). If consent was not
// granted, MaybeStartMetrics() instead clears the client ID, if any.
class BisonMetricsServiceClient : public metrics::MetricsServiceClient,
                               public metrics::EnabledStateProvider {
  friend struct base::LazyInstanceTraitsBase<BisonMetricsServiceClient>;

 public:
  static BisonMetricsServiceClient* GetInstance();

  BisonMetricsServiceClient();
  ~BisonMetricsServiceClient() override;

  void Initialize(PrefService* pref_service);
  void SetHaveMetricsConsent(bool user_consent, bool app_consent);
  std::unique_ptr<const base::FieldTrial::EntropyProvider>
  CreateLowEntropyProvider();

  // metrics::EnabledStateProvider
  bool IsConsentGiven() const override;
  bool IsReportingEnabled() const override;

  // metrics::MetricsServiceClient
  metrics::MetricsService* GetMetricsService() override;
  void SetMetricsClientId(const std::string& client_id) override;
  int32_t GetProduct() override;
  std::string GetApplicationLocale() override;
  bool GetBrand(std::string* brand_code) override;
  metrics::SystemProfileProto::Channel GetChannel() override;
  std::string GetVersionString() override;
  void CollectFinalMetricsForLog(const base::Closure& done_callback) override;
  std::unique_ptr<metrics::MetricsLogUploader> CreateUploader(
      const GURL& server_url,
      const GURL& insecure_server_url,
      base::StringPiece mime_type,
      metrics::MetricsLogUploader::MetricServiceType service_type,
      const metrics::MetricsLogUploader::UploadCallback& on_upload_complete)
      override;
  base::TimeDelta GetStandardUploadInterval() override;
  std::string GetAppPackageName() override;

 protected:
  virtual bool IsInSample();  // virtual for testing

 private:
  void MaybeStartMetrics();

  std::unique_ptr<metrics::MetricsStateManager> metrics_state_manager_;
  std::unique_ptr<metrics::MetricsService> metrics_service_;
  PrefService* pref_service_ = nullptr;
  bool init_finished_ = false;
  bool set_consent_finished_ = false;
  bool user_consent_ = false;
  bool app_consent_ = false;
  bool is_in_sample_ = false;

  // BisonMetricsServiceClient may be created before the UI thread is promoted to
  // BrowserThread::UI. Use |sequence_checker_| to enforce that the
  // BisonMetricsServiceClient is used on a single thread.
  base::SequenceChecker sequence_checker_;

  DISALLOW_COPY_AND_ASSIGN(BisonMetricsServiceClient);
};

}  // namespace bison

#endif  // BISON_BROWSER_METRICS_BISON_METRICS_SERVICE_CLIENT_H_