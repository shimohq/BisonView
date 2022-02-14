
#include "bison/browser/bv_browser_process.h"

#include "bison/browser/bv_browser_context.h"
#include "bison/browser/lifecycle/bv_contents_lifecycle_notifier.h"

#include "base/base_paths_posix.h"
#include "base/path_service.h"
#include "base/task/post_task.h"

#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace bison {

namespace prefs {

// String that specifies the Android account type to use for Negotiate
// authentication.
const char kAuthAndroidNegotiateAccountType[] =
    "auth.android_negotiate_account_type";

// Whitelist containing servers for which Integrated Authentication is enabled.
const char kAuthServerWhitelist[] = "auth.server_whitelist";

}  // namespace prefs

namespace {
BvBrowserProcess* g_bison_browser_process = nullptr;
}  // namespace

// static
BvBrowserProcess* BvBrowserProcess::GetInstance() {
  return g_bison_browser_process;
}

BvBrowserProcess::BvBrowserProcess(
    BvFeatureListCreator* bv_feature_list_creator) {
  g_bison_browser_process = this;
  bison_feature_list_creator_ = bv_feature_list_creator;
  bv_contents_lifecycle_notifier_ =
      std::make_unique<BvContentsLifecycleNotifier>(base::BindRepeating(
          &BvBrowserProcess::OnLoseForeground, base::Unretained(this)));
}

BvBrowserProcess::~BvBrowserProcess() {
  g_bison_browser_process = nullptr;
}

void BvBrowserProcess::PreMainMessageLoopRun() {
  pref_change_registrar_.Init(local_state());
  auto auth_pref_callback = base::BindRepeating(
      &BvBrowserProcess::OnAuthPrefsChanged, base::Unretained(this));
  pref_change_registrar_.Add(prefs::kAuthServerWhitelist,
  auth_pref_callback);
  pref_change_registrar_.Add(prefs::kAuthAndroidNegotiateAccountType,
                             auth_pref_callback);

  // jiang
  // InitSafeBrowsing();
}

PrefService* BvBrowserProcess::local_state() {
  if (!local_state_)
    CreateLocalState();
  return local_state_.get();
}

void BvBrowserProcess::CreateLocalState() {
  DCHECK(!local_state_);

  local_state_ = bison_feature_list_creator_->TakePrefService();
  DCHECK(local_state_);
}

void BvBrowserProcess::OnLoseForeground() {
  if (local_state_)
    local_state_->CommitPendingWrite();
}



BvBrowserPolicyConnector* BvBrowserProcess::browser_policy_connector() {
  if (!browser_policy_connector_)
    CreateBrowserPolicyConnector();
  return browser_policy_connector_.get();
}

void BvBrowserProcess::CreateBrowserPolicyConnector() {
  DCHECK(!browser_policy_connector_);

  browser_policy_connector_ =
      bison_feature_list_creator_->TakeBrowserPolicyConnector();
  DCHECK(browser_policy_connector_);
}

// static
void BvBrowserProcess::RegisterNetworkContextLocalStatePrefs(
    PrefRegistrySimple* pref_registry) {
  pref_registry->RegisterStringPref(prefs::kAuthServerWhitelist,
  std::string());
  pref_registry->RegisterStringPref(prefs::kAuthAndroidNegotiateAccountType,
                                    std::string());
}

network::mojom::HttpAuthDynamicParamsPtr
BvBrowserProcess::CreateHttpAuthDynamicParams() {
  network::mojom::HttpAuthDynamicParamsPtr auth_dynamic_params =
      network::mojom::HttpAuthDynamicParams::New();

  auth_dynamic_params->server_allowlist =
      local_state()->GetString(prefs::kAuthServerWhitelist);
  auth_dynamic_params->android_negotiate_account_type =
      local_state()->GetString(prefs::kAuthAndroidNegotiateAccountType);

  auth_dynamic_params->ntlm_v2_enabled = true;

  return auth_dynamic_params;
}

void BvBrowserProcess::OnAuthPrefsChanged() {
  content::GetNetworkService()->ConfigureHttpAuthPrefs(
      CreateHttpAuthDynamicParams());
}

}  // namespace bison
