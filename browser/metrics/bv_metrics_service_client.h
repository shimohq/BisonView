// create by jiang947

#ifndef BISON_BROWSER_METRICS_BV_METRICS_SERVICE_CLIENT_H_
#define BISON_BROWSER_METRICS_BV_METRICS_SERVICE_CLIENT_H_



#include <memory>
#include <string>

#include "bison/browser/lifecycle/webview_app_state_observer.h"

#include "base/metrics/field_trial.h"
#include "base/no_destructor.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "components/embedder_support/android/metrics/android_metrics_service_client.h"
#include "components/metrics/enabled_state_provider.h"
#include "components/metrics/metrics_log_uploader.h"
#include "components/metrics/metrics_service_client.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace bison {
namespace prefs {
extern const char kMetricsAppPackageNameLoggingRule[];
extern const char kAppPackageNameLoggingRuleLastUpdateTime[];
}  // namespace prefs

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


class BvMetricsServiceClient : public metrics::MetricsServiceClient,
                               public WebViewAppStateObserver {
  friend class base::NoDestructor<BvMetricsServiceClient>;

 public:
  // This interface define the tasks that depend on the
  // android_webview/browser directory.
  class Delegate {
   public:
    Delegate();
    virtual ~Delegate();

    // Not copyable or movable
    Delegate(const Delegate&) = delete;
    Delegate& operator=(const Delegate&) = delete;
    Delegate(Delegate&&) = delete;
    Delegate& operator=(Delegate&&) = delete;

    virtual void RegisterAdditionalMetricsProviders(
        metrics::MetricsService* service) = 0;
    virtual void AddWebViewAppStateObserver(
        WebViewAppStateObserver* observer) = 0;
    virtual bool HasAwContentsEverCreated() const = 0;
  };

  //   virtual void RegisterAdditionalMetricsProviders(
  //       metrics::MetricsService* service) = 0;
  //   virtual void AddWebViewAppStateObserver(
  //       WebViewAppStateObserver* observer) = 0;
  //   virtual bool HasContentsEverCreated() const = 0;
  // };

  static BvMetricsServiceClient* GetInstance();
  static void SetInstance(
      std::unique_ptr<BvMetricsServiceClient> aw_metrics_service_client);

  static void RegisterMetricsPrefs(PrefRegistrySimple* registry);

  BvMetricsServiceClient(std::unique_ptr<Delegate> delegate);

  BvMetricsServiceClient(const BvMetricsServiceClient&) = delete;
  BvMetricsServiceClient& operator=(const BvMetricsServiceClient&) = delete;

  ~BvMetricsServiceClient() override;

  // metrics::MetricsServiceClient
  int32_t GetProduct() override;

  // WebViewAppStateObserver
  void OnAppStateChanged(WebViewAppStateObserver::State state) override;

  // metrics::AndroidMetricsServiceClient:
  void OnMetricsStart() override;
  void OnMetricsNotStarted() override;
  int GetSampleRatePerMille() const override;
  int GetPackageNameLimitRatePerMille() override;
  void RegisterAdditionalMetricsProviders(
      metrics::MetricsService* service) override;

  // Gets the embedding app's package name if it's OK to log. Otherwise, this
  // returns the empty string.
  std::string GetAppPackageNameIfLoggable() override;

  // If `android_webview::features::kWebViewAppsPackageNamesAllowlist` is
  // enabled:
  // - It returns `true` if the app is in the list of allowed apps.
  // - It returns `false` if the app isn't in the allowlist or if the lookup
  //   operation fails or hasn't finished yet.
  //
  // If the feature isn't enabled, the default sampling behaviour in
  // `::metrics::AndroidMetricsServiceClient::ShouldRecordPackageName` is used.
  bool ShouldRecordPackageName() override;

  // Sets that the embedding app's package name is allowed to be recorded in
  // UMA logs. This is determened by looking up the app package name in a
  // dynamically downloaded allowlist of apps see
  // `AwAppsPackageNamesAllowlistComponentLoaderPolicy`.
  //
  // `record` If it has a null value, then it will be ignored and the cached
  //          record will be used if any.
  void SetAppPackageNameLoggingRule(
      absl::optional<AppPackageNameLoggingRule> record);

  // Get the cached record of the app package names allowlist set by
  // `SetAppPackageNameLoggingRule` if any.
  absl::optional<AppPackageNameLoggingRule>
  GetCachedAppPackageNameLoggingRule();

  // The last time the apps package name allowlist was queried from the
  // component update service, regardless if it was successful or not.
  base::Time GetAppPackageNameLoggingRuleLastUpdateTime() const;
  void SetAppPackageNameLoggingRuleLastUpdateTime(base::Time update_time);

 protected:
  // Restrict usage of the inherited AndroidMetricsServiceClient::RegisterPrefs,
  // RegisterMetricsPrefs should be used instead.
  using AndroidMetricsServiceClient::RegisterPrefs;

 private:
  bool app_in_foreground_ = false;
  base::Time time_created_;
  std::unique_ptr<Delegate> delegate_;

  absl::optional<AppPackageNameLoggingRule> cached_package_name_record_;
  AppPackageNameLoggingRuleStatus package_name_record_status_ =
      AppPackageNameLoggingRuleStatus::kNotLoadedNoCache;
};

}  // namespace bison

#endif  // BISON_BROWSER_METRICS_BV_METRICS_SERVICE_CLIENT_H_
