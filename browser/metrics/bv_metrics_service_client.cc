
#include "bison/browser/metrics/bv_metrics_service_client.h"

#include <jni.h>
#include <cstdint>

#include "bison/bison_jni_headers/BvMetricsServiceClient_jni.h"
#include "bison/common/bv_features.h"
#include "bison/common/metrics/app_package_name_logging_rule.h"

#include "base/android/callback_android.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/persistent_histogram_allocator.h"
#include "base/time/time.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/metrics/metrics_service.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/android/channel_getter.h"

namespace bison {
namespace prefs {
const char kMetricsAppPackageNameLoggingRule[] =
    "aw_metrics_app_package_name_logging_rule";
const char kAppPackageNameLoggingRuleLastUpdateTime[] =
    "aw_metrics_app_package_name_logging_rule_last_update";
}  // namespace prefs

namespace {

// IMPORTANT: DO NOT CHANGE sample rates without first ensuring the Chrome
// Metrics team has the appropriate backend bandwidth and storage.

// Sample at 2%, based on storage concerns. We sample at a different rate than
// Chrome because we have more metrics "clients" (each app on the device counts
// as a separate client).
const int kStableSampledInRatePerMille = 20;

// Sample non-stable channels at 99%, to boost volume for pre-stable
// experiments. We choose 99% instead of 100% for consistency with Chrome and to
// exercise the out-of-sample code path.
const int kBetaDevCanarySampledInRatePerMille = 990;

// The fraction of UMA clients for whom package name data is uploaded. This
// threshold and the corresponding privacy requirements are described in more
// detail at http://shortn/_CzfDUxTxm2 (internal document). We also have public
// documentation for metrics collection in WebView more generally (see
// https://developer.android.com/guide/webapps/webview-privacy).
//
// Do not change this constant without seeking privacy approval with the teams
// outlined in the internal document above.
const int kPackageNameLimitRatePerMille = 100;  // (10% of UMA clients)

BvMetricsServiceClient* g_bv_metrics_service_client = nullptr;

}  // namespace

BvMetricsServiceClient::Delegate::Delegate() = default;
BvMetricsServiceClient::Delegate::~Delegate() = default;

// static
BvMetricsServiceClient* BvMetricsServiceClient::GetInstance() {
  DCHECK(g_bv_metrics_service_client);
  g_bv_metrics_service_client->EnsureOnValidSequence();
  return g_bv_metrics_service_client;
}

// static
void BvMetricsServiceClient::SetInstance(
    std::unique_ptr<BvMetricsServiceClient> bv_metrics_service_client) {
  DCHECK(!g_bv_metrics_service_client);
  DCHECK(bv_metrics_service_client);
  g_bv_metrics_service_client = bv_metrics_service_client.release();
  g_bv_metrics_service_client->EnsureOnValidSequence();
}

BvMetricsServiceClient::BvMetricsServiceClient(
    std::unique_ptr<Delegate> delegate)
    : time_created_(base::Time::Now()), delegate_(std::move(delegate)) {}

BvMetricsServiceClient::~BvMetricsServiceClient() = default;

int32_t BvMetricsServiceClient::GetProduct() {
  return metrics::ChromeUserMetricsExtension::ANDROID_WEBVIEW;
}

int BvMetricsServiceClient::GetSampleRatePerMille() const {
  // Down-sample unknown channel as a precaution in case it ends up being
  // shipped to Stable users.
  version_info::Channel channel = version_info::android::GetChannel();
  if (channel == version_info::Channel::STABLE ||
      channel == version_info::Channel::UNKNOWN) {
    return kStableSampledInRatePerMille;
  }
  return kBetaDevCanarySampledInRatePerMille;
  }

std::string BvMetricsServiceClient::GetAppPackageNameIfLoggable() {
  AndroidMetricsServiceClient::InstallerPackageType installer_type =
      GetInstallerPackageType();
  // Always record the app package name of system apps even if it's not in the
  // allowlist.
  if (installer_type == InstallerPackageType::SYSTEM_APP ||
      (installer_type == InstallerPackageType::GOOGLE_PLAY_STORE &&
       ShouldRecordPackageName())) {
    return GetAppPackageName();
  }
  return std::string();
}

bool BvMetricsServiceClient::ShouldRecordPackageName() {
  base::UmaHistogramEnumeration(
      "BisonView.Metrics.PackagesAllowList.RecordStatus",
      package_name_record_status_);
  return cached_package_name_record_.has_value() &&
         cached_package_name_record_.value().IsAppPackageNameAllowed();
}

void BvMetricsServiceClient::SetAppPackageNameLoggingRule(
    absl::optional<AppPackageNameLoggingRule> record) {
  absl::optional<AppPackageNameLoggingRule> cached_record =
      GetCachedAppPackageNameLoggingRule();
  if (!record.has_value()) {
    package_name_record_status_ =
        cached_record.has_value()
            ? AppPackageNameLoggingRuleStatus::kNewVersionFailedUseCache
            : AppPackageNameLoggingRuleStatus::kNewVersionFailedNoCache;
    return;
  }

  if (cached_record.has_value() &&
      record.value().IsSameAs(cached_package_name_record_.value())) {
    package_name_record_status_ =
        AppPackageNameLoggingRuleStatus::kSameVersionAsCache;
    return;
  }

  PrefService* local_state = pref_service();
  DCHECK(local_state);
  local_state->Set(prefs::kMetricsAppPackageNameLoggingRule,
                   record.value().ToDictionary());
  cached_package_name_record_ = record;
  package_name_record_status_ =
      AppPackageNameLoggingRuleStatus::kNewVersionLoaded;

  UmaHistogramTimes(
      "BisonView.Metrics.PackagesAllowList.ResultReceivingDelay",
      base::Time::Now() - time_created_);
}

absl::optional<AppPackageNameLoggingRule>
BvMetricsServiceClient::GetCachedAppPackageNameLoggingRule() {
  if (cached_package_name_record_.has_value()) {
    return cached_package_name_record_;
}

  PrefService* local_state = pref_service();
  DCHECK(local_state);
  cached_package_name_record_ = AppPackageNameLoggingRule::FromDictionary(
      local_state->GetValue(prefs::kMetricsAppPackageNameLoggingRule));
  if (cached_package_name_record_.has_value()) {
    package_name_record_status_ =
        AppPackageNameLoggingRuleStatus::kNotLoadedUseCache;
  }
  return cached_package_name_record_;
}

base::Time BvMetricsServiceClient::GetAppPackageNameLoggingRuleLastUpdateTime()
    const {
  PrefService* local_state = pref_service();
  DCHECK(local_state);
  return local_state->GetTime(prefs::kAppPackageNameLoggingRuleLastUpdateTime);
}

void BvMetricsServiceClient::SetAppPackageNameLoggingRuleLastUpdateTime(
    base::Time update_time) {
  PrefService* local_state = pref_service();
  DCHECK(local_state);
  local_state->SetTime(prefs::kAppPackageNameLoggingRuleLastUpdateTime,
                       update_time);
}

void BvMetricsServiceClient::OnMetricsStart() {
  delegate_->AddWebViewAppStateObserver(this);
}

void BvMetricsServiceClient::OnMetricsNotStarted() {}

int BvMetricsServiceClient::GetPackageNameLimitRatePerMille() {
  return kPackageNameLimitRatePerMille;
}

void BvMetricsServiceClient::OnAppStateChanged(
    WebViewAppStateObserver::State state) {
  // To match MetricsService's expectation,
  // - does nothing if no WebView has ever been created.
  // - starts notifying MetricsService once a WebView is created and the app
  //   is foreground.
  // - consolidates the other states other than kForeground into background.
  // - avoids the duplicated notification.
  if (state == WebViewAppStateObserver::State::kDestroyed &&
      !delegate_->HasBvContentsEverCreated()) {
    return;
}

  bool foreground = state == WebViewAppStateObserver::State::kForeground;

  if (foreground == app_in_foreground_)
    return;

  app_in_foreground_ = foreground;
  if (app_in_foreground_) {
    GetMetricsService()->OnAppEnterForeground();
  } else {
    // TODO(https://crbug.com/1052392): Turn on the background recording.
    // Not recording in background, this matches Chrome's behavior.
    GetMetricsService()->OnAppEnterBackground(
        /* keep_recording_in_background = false */);
  }
}

void BvMetricsServiceClient::RegisterAdditionalMetricsProviders(
    metrics::MetricsService* service) {
  delegate_->RegisterAdditionalMetricsProviders(service);
}

// static
void BvMetricsServiceClient::RegisterMetricsPrefs(
    PrefRegistrySimple* registry) {
  RegisterPrefs(registry);
  registry->RegisterDictionaryPref(prefs::kMetricsAppPackageNameLoggingRule,
                                   base::Value(base::Value::Type::DICTIONARY));
  registry->RegisterTimePref(prefs::kAppPackageNameLoggingRuleLastUpdateTime,
                             base::Time());
}

// static
void JNI_BvMetricsServiceClient_SetHaveMetricsConsent(JNIEnv* env,
                                                      jboolean user_consent,
                                                      jboolean app_consent) {
  BvMetricsServiceClient::GetInstance()->SetHaveMetricsConsent(user_consent,
                                                               app_consent);
}

// static
void JNI_BvMetricsServiceClient_SetFastStartupForTesting(
    JNIEnv* env,
    jboolean fast_startup_for_testing) {
  BvMetricsServiceClient::GetInstance()->SetFastStartupForTesting(
      fast_startup_for_testing);
}

// static
void JNI_BvMetricsServiceClient_SetUploadIntervalForTesting(
    JNIEnv* env,
    jlong upload_interval_ms) {
  BvMetricsServiceClient::GetInstance()->SetUploadIntervalForTesting(
      base::Milliseconds(upload_interval_ms));
}

// static
void JNI_BvMetricsServiceClient_SetOnFinalMetricsCollectedListenerForTesting(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& listener) {
  BvMetricsServiceClient::GetInstance()
      ->SetOnFinalMetricsCollectedListenerForTesting(base::BindRepeating(
          base::android::RunRunnableAndroid,
          base::android::ScopedJavaGlobalRef<jobject>(listener)));
}

// static
void JNI_BvMetricsServiceClient_SetAppPackageNameLoggingRuleForTesting(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& version,
    jlong expiry_date_ms) {
  BvMetricsServiceClient::GetInstance()->SetAppPackageNameLoggingRule(
      AppPackageNameLoggingRule(
          base::Version(base::android::ConvertJavaStringToUTF8(env, version)),
          base::Time::UnixEpoch() + base::Milliseconds(expiry_date_ms)));
}

}  // namespace bison
