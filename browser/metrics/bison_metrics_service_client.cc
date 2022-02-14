
#include "bison/browser/metrics/bison_metrics_service_client.h"

#include <jni.h>
#include <cstdint>
#include <memory>

#include "bison/browser/metrics/bison_metrics_log_uploader.h"
#include "bison/bison_jni_headers/BvMetricsServiceClient_jni.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/hash/hash.h"
#include "base/i18n/rtl.h"
#include "base/lazy_instance.h"
#include "base/metrics/histogram_functions.h"
#include "base/no_destructor.h"
#include "base/strings/string16.h"
#include "components/metrics/call_stack_profile_metrics_provider.h"
#include "components/metrics/cpu_metrics_provider.h"
#include "components/metrics/enabled_state_provider.h"
#include "components/metrics/metrics_log_uploader.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/metrics/metrics_service.h"
#include "components/metrics/metrics_state_manager.h"
#include "components/metrics/net/cellular_logic_helper.h"
#include "components/metrics/net/network_metrics_provider.h"
#include "components/metrics/ui/screen_info_metrics_provider.h"
#include "components/metrics/version_utils.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/android/channel_getter.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/network_service_instance.h"

namespace bison {

namespace {

// IMPORTANT: DO NOT CHANGE sample rates without first ensuring the Chrome
// Metrics team has the appropriate backend bandwidth and storage.

// Sample at 2%, based on storage concerns. We sample at a different rate than
// Chrome because we have more metrics "clients" (each app on the device counts
// as a separate client).
const double kStableSampledInRate = 0.02;

// Sample non-stable channels also at 2%. We intend to raise this to 99% in the
// future (for consistency with Chrome and to exercise the out-of-sample code
// path).
const double kBetaDevCanarySampledInRate = 0.02;

// Callbacks for metrics::MetricsStateManager::Create. Store/LoadClientInfo
// allow Windows Chrome to back up ClientInfo. They're no-ops for WebView.
BvMetricsServiceClient* g_bison_metrics_service_client = nullptr;

void StoreClientInfo(const metrics::ClientInfo& client_info) {}

std::unique_ptr<metrics::ClientInfo> LoadClientInfo() {
  std::unique_ptr<metrics::ClientInfo> client_info;
  return client_info;
}

// WebView metrics are sampled at (possibly) different rates depending on
// channel, based on the client ID. Sampling is hard-coded (rather than
// controlled via variations, as in Chrome) because:
// - WebView is slow to download the variations seed and propagate it to each
//   app, so we'd miss metrics from the first few runs of each app.
// - WebView uses the low-entropy source for all studies, so there would be
//   crosstalk between the metrics sampling study and all other studies.
bool IsInSample(const std::string& client_id) {
  DCHECK(!client_id.empty());

  double sampled_in_rate = kBetaDevCanarySampledInRate;

  // Down-sample unknown channel as a precaution in case it ends up being
  // shipped to Stable users.
  version_info::Channel channel = version_info::android::GetChannel();
  if (channel == version_info::Channel::STABLE ||
      channel == version_info::Channel::UNKNOWN) {
    sampled_in_rate = kStableSampledInRate;
  }

  // client_id comes from base::GenerateGUID(), so its value is random/uniform,
  // except for a few bit positions with fixed values, and some hyphens. Rather
  // than separating the random payload from the fixed bits, just hash the whole
  // thing, to produce a new random/~uniform value.
  uint32_t hash = base::PersistentHash(client_id);

  // Since hashing is ~uniform, the chance that the value falls in the bottom
  // X% of possible values is X%. UINT32_MAX fits within the range of integers
  // that can be expressed precisely by a 64-bit double. Casting back to a
  // uint32_t means the effective sample rate is within a 1/UINT32_MAX error
  // margin.
  uint32_t sampled_in_threshold =
      static_cast<uint32_t>(static_cast<double>(UINT32_MAX) * sampled_in_rate);
  return hash < sampled_in_threshold;
}

std::unique_ptr<metrics::MetricsService> CreateMetricsService(
    metrics::MetricsStateManager* state_manager,
    metrics::MetricsServiceClient* client,
    PrefService* prefs) {
  auto service =
      std::make_unique<metrics::MetricsService>(state_manager, client, prefs);
  service->RegisterMetricsProvider(
      std::make_unique<metrics::NetworkMetricsProvider>(
          content::CreateNetworkConnectionTrackerAsyncGetter()));
  service->RegisterMetricsProvider(
      std::make_unique<metrics::CPUMetricsProvider>());
  service->RegisterMetricsProvider(
      std::make_unique<metrics::ScreenInfoMetricsProvider>());
  service->RegisterMetricsProvider(
      std::make_unique<metrics::CallStackProfileMetricsProvider>());
  service->InitializeMetricsRecordingState();
  return service;
}

// Queries the system for the app's first install time and uses this in the
// kInstallDate pref. Must be called before created a MetricsStateManager.
// TODO(https://crbug.com/1012025): remove this when the kInstallDate pref has
// been persisted for one or two milestones.
void PopulateSystemInstallDateIfNecessary(PrefService* prefs) {
  int64_t install_date = prefs->GetInt64(metrics::prefs::kInstallDate);
  if (install_date > 0) {
    // kInstallDate appears to be valid (common case). Finish early as an
    // optimization to avoid a JNI call below.
    base::UmaHistogramEnumeration("Android.WebView.Metrics.BackfillInstallDate",
                                  BackfillInstallDate::kValidInstallDatePref);
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  int64_t system_install_date =
      Java_BvMetricsServiceClient_getAppInstallTime(env);
  if (system_install_date < 0) {
    // Could not figure out install date from the system. Let the
    // MetricsStateManager set this pref to its best guess for a reasonable
    // time.
    base::UmaHistogramEnumeration(
        "Android.WebView.Metrics.BackfillInstallDate",
        BackfillInstallDate::kCouldNotGetPackageManagerInstallDate);
    return;
  }

  base::UmaHistogramEnumeration(
      "Android.WebView.Metrics.BackfillInstallDate",
      BackfillInstallDate::kPersistedPackageManagerInstallDate);
  prefs->SetInt64(metrics::prefs::kInstallDate, system_install_date);
}

}  // namespace



BvMetricsServiceClient::BvMetricsServiceClient() {}
BvMetricsServiceClient::~BvMetricsServiceClient() {}



// static
BvMetricsServiceClient* BvMetricsServiceClient::GetInstance() {
  DCHECK(g_bison_metrics_service_client);
  g_bison_metrics_service_client->EnsureOnValidSequence();
  return g_bison_metrics_service_client;
}

int32_t BvMetricsServiceClient::GetProduct() {
  return metrics::ChromeUserMetricsExtension::ANDROID_WEBVIEW;
}

std::string BvMetricsServiceClient::GetAppPackageName() {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> j_app_name =
      Java_BvMetricsServiceClient_getAppPackageName(env);
  if (j_app_name)
    return ConvertJavaStringToUTF8(env, j_app_name);
  return std::string();
}

bool BvMetricsServiceClient::IsInSample() {
  // Called in MaybeStartMetrics(), after metrics_service_ is created.
  return ::bison::IsInSample(metrics_service_->GetClientId());
}

// static
void JNI_BvMetricsServiceClient_SetHaveMetricsConsent(JNIEnv* env,
                                                      jboolean user_consent,
                                                      jboolean app_consent) {
}

}  // namespace bison
