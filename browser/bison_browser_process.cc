
#include "bison/browser/bison_browser_process.h"

#include "base/base_paths_posix.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "bison/browser/bison_browser_context.h"
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
BisonBrowserProcess* g_bison_browser_process = nullptr;
}  // namespace

// static
BisonBrowserProcess* BisonBrowserProcess::GetInstance() {
  return g_bison_browser_process;
}

BisonBrowserProcess::BisonBrowserProcess(
    BisonFeatureListCreator* bison_feature_list_creator) {
  g_bison_browser_process = this;
  bison_feature_list_creator_ = bison_feature_list_creator;
}

BisonBrowserProcess::~BisonBrowserProcess() {
  g_bison_browser_process = nullptr;
}

void BisonBrowserProcess::PreMainMessageLoopRun() {
  pref_change_registrar_.Init(local_state());
  auto auth_pref_callback = base::BindRepeating(
      &BisonBrowserProcess::OnAuthPrefsChanged, base::Unretained(this));
  pref_change_registrar_.Add(prefs::kAuthServerWhitelist,
  auth_pref_callback);
  pref_change_registrar_.Add(prefs::kAuthAndroidNegotiateAccountType,
                             auth_pref_callback);

  // jiang
  // InitSafeBrowsing();
}

PrefService* BisonBrowserProcess::local_state() {
  if (!local_state_)
    CreateLocalState();
  return local_state_.get();
}

void BisonBrowserProcess::CreateLocalState() {
  DCHECK(!local_state_);

  local_state_ = bison_feature_list_creator_->TakePrefService();
  DCHECK(local_state_);
}

BisonBrowserPolicyConnector* BisonBrowserProcess::browser_policy_connector() {
  if (!browser_policy_connector_)
    CreateBrowserPolicyConnector();
  return browser_policy_connector_.get();
}

void BisonBrowserProcess::CreateBrowserPolicyConnector() {
  DCHECK(!browser_policy_connector_);

  browser_policy_connector_ =
      bison_feature_list_creator_->TakeBrowserPolicyConnector();
  DCHECK(browser_policy_connector_);
}

// static
void BisonBrowserProcess::RegisterNetworkContextLocalStatePrefs(
    PrefRegistrySimple* pref_registry) {
  pref_registry->RegisterStringPref(prefs::kAuthServerWhitelist,
  std::string());
  pref_registry->RegisterStringPref(prefs::kAuthAndroidNegotiateAccountType,
                                    std::string());
}

network::mojom::HttpAuthDynamicParamsPtr
BisonBrowserProcess::CreateHttpAuthDynamicParams() {
  network::mojom::HttpAuthDynamicParamsPtr auth_dynamic_params =
      network::mojom::HttpAuthDynamicParams::New();

  auth_dynamic_params->server_allowlist =
      local_state()->GetString(prefs::kAuthServerWhitelist);
  auth_dynamic_params->android_negotiate_account_type =
      local_state()->GetString(prefs::kAuthAndroidNegotiateAccountType);

  auth_dynamic_params->ntlm_v2_enabled = true;

  return auth_dynamic_params;
}

void BisonBrowserProcess::OnAuthPrefsChanged() {
  content::GetNetworkService()->ConfigureHttpAuthPrefs(
      CreateHttpAuthDynamicParams());
}

}  // namespace bison
