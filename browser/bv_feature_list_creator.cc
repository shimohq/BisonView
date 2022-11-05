#include "bison/browser/bv_feature_list_creator.h"

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "components/metrics/metrics_service.h"


#include "bison/browser/bison_variations_seed_bridge.h"
#include "bison/browser/bv_browser_context.h"
#include "bison/browser/bv_browser_process.h"
#include "bison/browser/bv_pref_names.h"
// #include "bison/browser/metrics/bison_metrics_service_client.h"

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "components/autofill/core/common/autofill_prefs.h"
#include "components/embedder_support/android/metrics/android_metrics_service_client.h"
#include "components/embedder_support/origin_trials/origin_trial_prefs.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/metrics/persistent_histograms.h"
#include "components/policy/core/browser/configuration_policy_pref_store.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/in_memory_pref_store.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/prefs/segregated_pref_store.h"
#include "components/variations/entropy_provider.h"
#include "components/variations/pref_names.h"
#include "components/variations/service/safe_seed_manager.h"
#include "components/variations/service/variations_service.h"
#include "content/public/common/content_switch_dependent_feature_overrides.h"
#include "net/base/features.h"
#include "net/nqe/pref_names.h"

namespace bison {

namespace {


// These prefs go in the JsonPrefStore, and will persist across runs. Other
// prefs go in the InMemoryPrefStore, and will be lost when the process ends.
const char* const kPersistentPrefsAllowlist[] = {
    // Randomly-generated GUID which pseudonymously identifies uploaded metrics.
    metrics::prefs::kMetricsClientID,
    // Random seed value for variation's entropy providers. Used to assign
    // experiment groups.
    metrics::prefs::kMetricsLowEntropySource,
    // File metrics metadata.
    metrics::prefs::kMetricsFileMetricsMetadata,
    // Logged directly in the ChromeUserMetricsExtension proto.
    metrics::prefs::kInstallDate,
    metrics::prefs::kMetricsReportingEnabledTimestamp,
    metrics::prefs::kMetricsSessionID,
    // Logged in system_profile.stability fields.
    metrics::prefs::kStabilityFileMetricsUnsentFilesCount,
    metrics::prefs::kStabilityFileMetricsUnsentSamplesCount,
    metrics::prefs::kStabilityLaunchCount,
    metrics::prefs::kStabilityPageLoadCount,
    metrics::prefs::kStabilityRendererLaunchCount,
    // Unsent logs.
    metrics::prefs::kMetricsInitialLogs, metrics::prefs::kMetricsOngoingLogs,
    // Unsent logs metadata.
    metrics::prefs::kMetricsInitialLogsMetadata,
    metrics::prefs::kMetricsOngoingLogsMetadata, net::nqe::kNetworkQualities,
    // Current and past country codes, to filter variations studies by country.
    variations::prefs::kVariationsCountry,
    variations::prefs::kVariationsPermanentConsistencyCountry,
    // Last variations seed fetch date/time, used for histograms and to
    // determine if the seed is expired.
    variations::prefs::kVariationsLastFetchTime,
    variations::prefs::kVariationsSeedDate,

    // A dictionary that caches 'AppPackageNameLoggingRule' object which decides
    // whether the app package name should be recorded in UMA or not.
    // prefs::kMetricsAppPackageNameLoggingRule,

    // The last time the apps package name allowlist was queried from the
    // component update service, regardless if it was successful or not.
    // prefs::kAppPackageNameLoggingRuleLastUpdateTime,
};

void HandleReadError(PersistentPrefStore::PrefReadError error) {}

base::FilePath GetPrefStorePath() {
  base::FilePath path;
  base::PathService::Get(base::DIR_ANDROID_APP_DATA, &path);
  path = path.Append(FILE_PATH_LITERAL("pref_store"));
  return path;
}



}  // namespace

BvFeatureListCreator::BvFeatureListCreator()
    : bv_field_trials_(std::make_unique<BvFieldTrials>()) {}

BvFeatureListCreator::~BvFeatureListCreator() {}

std::unique_ptr<PrefService> BvFeatureListCreator::CreatePrefService() {
  //auto pref_registry = base::MakeRefCounted<PrefRegistrySimple>(); //shell
  auto pref_registry = base::MakeRefCounted<user_prefs::PrefRegistrySyncable>();

  metrics::MetricsService::RegisterPrefs(pref_registry.get());
  variations::VariationsService::RegisterPrefs(pref_registry.get());

  embedder_support::OriginTrialPrefs::RegisterPrefs(pref_registry.get());
  BvBrowserProcess::RegisterNetworkContextLocalStatePrefs(pref_registry.get());
  BvBrowserProcess::RegisterEnterpriseAuthenticationAppLinkPolicyPref(
      pref_registry.get());

  PrefServiceFactory pref_service_factory;

  std::set<std::string> persistent_prefs;
  for (const char* const pref_name : kPersistentPrefsAllowlist)
    persistent_prefs.insert(pref_name);

  // SegregatedPrefStore may be validated with a MAC (message authentication
  // code). On Android, the store is protected by app sandboxing, so validation
  // is unnnecessary. Thus validation_delegate is null.
  pref_service_factory.set_user_prefs(base::MakeRefCounted<SegregatedPrefStore>(
      base::MakeRefCounted<InMemoryPrefStore>(),
      base::MakeRefCounted<JsonPrefStore>(GetPrefStorePath()),
      std::move(persistent_prefs)));

  pref_service_factory.set_managed_prefs(
      base::MakeRefCounted<policy::ConfigurationPolicyPrefStore>(
          browser_policy_connector_.get(),
          browser_policy_connector_->GetPolicyService(),
          browser_policy_connector_->GetHandlerList(),
          policy::POLICY_LEVEL_MANDATORY));

  pref_service_factory.set_read_error_callback(
      base::BindRepeating(&HandleReadError));

  return pref_service_factory.Create(pref_registry);
}


void BvFeatureListCreator::SetUpFieldTrials() {
  //DCHECK(base::FieldTrialList::GetInstance());
  // std::unique_ptr<BvVariationsSeed> seed_proto = TakeSeed();
  // std::unique_ptr<variations::SeedResponse> seed;
  // base::Time seed_date;  // Initializes to null time.
  // if (seed_proto) {
  //   seed = std::make_unique<variations::SeedResponse>();
  //   seed->data = seed_proto->seed_data();
  //   seed->signature = seed_proto->signature();
  //   seed->country = seed_proto->country();
  //   seed->date = seed_proto->date();
  //   seed->is_gzip_compressed = seed_proto->is_gzip_compressed();

  //   // We set the seed fetch time to when the service downloaded the seed rather
  //   // than base::Time::Now() because we want to compute seed freshness based on
  //   // the initial download time, which happened in the service at some earlier
  //   // point.
  //   seed_date = base::Time::FromJavaTime(seed->date);
  // }

  // client_ = std::make_unique<AwVariationsServiceClient>();
  // auto seed_store = std::make_unique<variations::VariationsSeedStore>(
  //     local_state_.get(), /*initial_seed=*/std::move(seed),
  //     /*signature_verification_enabled=*/g_signature_verification_enabled,
  //     /*use_first_run_prefs=*/false);

  // if (!seed_date.is_null())
  //   seed_store->RecordLastFetchTime(seed_date);

  // variations::UIStringOverrider ui_string_overrider;
  // variations_field_trial_creator_ =
  //     std::make_unique<variations::VariationsFieldTrialCreator>(
  //         client_.get(), std::move(seed_store), ui_string_overrider);
  // variations_field_trial_creator_->OverrideVariationsPlatform(
  //     variations::Study::PLATFORM_ANDROID_WEBVIEW);

  // // Safe Mode is a feature which reverts to a previous variations seed if the
  // // current one is suspected to be causing crashes, or preventing new seeds
  // // from being downloaded. It's not implemented for WebView because 1) it's
  // // difficult for WebView to implement Safe Mode's crash detection, and 2)
  // // downloading and disseminating seeds is handled by the WebView service,
  // // which itself doesn't support variations; therefore a bad seed shouldn't be
  // // able to break seed downloads. See https://crbug.com/801771 for more info.
  // variations::SafeSeedManager ignored_safe_seed_manager(local_state_.get());

  // auto feature_list = std::make_unique<base::FeatureList>();
  // std::vector<std::string> variation_ids =
  //     aw_feature_entries::RegisterEnabledFeatureEntries(feature_list.get());

  // auto* metrics_client = AwMetricsServiceClient::GetInstance();
  // // Populate FieldTrialList. Since |low_entropy_provider| is null, it will fall
  // // back to the provider we previously gave to FieldTrialList, which is a low
  // // entropy provider. The X-Client-Data header is not reported on WebView, so
  // // we pass an empty object as the |low_entropy_source_value|.
  // variations_field_trial_creator_->SetUpFieldTrials(
  //     variation_ids,
  //     GetSwitchDependentFeatureOverrides(
  //         *base::CommandLine::ForCurrentProcess()),
  //     /*low_entropy_provider=*/nullptr, std::move(feature_list),
  //     metrics_client->metrics_state_manager(), aw_field_trials_.get(),
  //     &ignored_safe_seed_manager, /*low_entropy_source_value=*/absl::nullopt);
}

void BvFeatureListCreator::CreateLocalState() {
  browser_policy_connector_ = std::make_unique<BvBrowserPolicyConnector>();
  local_state_ = CreatePrefService();
}

void BvFeatureListCreator::CreateFeatureListAndFieldTrials() {
  CreateLocalState();
  // jiang
  // BvMetricsServiceClient::GetInstance()->Initialize(local_state_.get());
  SetUpFieldTrials();
}

}  // namespace bison
