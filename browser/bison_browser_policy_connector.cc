#include "bison/browser/bison_browser_policy_connector.h"

#include <memory>

#include "bison/browser/bison_browser_process.h"
#include "base/bind.h"
#include "components/policy/core/browser/android/android_combined_policy_provider.h"
#include "components/policy/core/browser/configuration_policy_handler_list.h"
#include "components/policy/core/browser/url_blacklist_policy_handler.h"
#include "components/policy/core/common/policy_pref_names.h"
#include "components/policy/policy_constants.h"
#include "net/url_request/url_request_context_getter.h"

namespace bison {

namespace {

// Callback only used in ChromeOS. No-op here.
void PopulatePolicyHandlerParameters(
    policy::PolicyHandlerParameters* parameters) {}

// Used to check if a policy is deprecated. Currently bypasses that check.
const policy::PolicyDetails* GetChromePolicyDetails(const std::string& policy) {
  return nullptr;
}

// Factory for the handlers that will be responsible for converting the policies
// to the associated preferences.
std::unique_ptr<policy::ConfigurationPolicyHandlerList> BuildHandlerList(
    const policy::Schema& chrome_schema) {
  std::unique_ptr<policy::ConfigurationPolicyHandlerList> handlers(
      new policy::ConfigurationPolicyHandlerList(
          base::BindRepeating(&PopulatePolicyHandlerParameters),
          base::BindRepeating(&GetChromePolicyDetails),
          false));

  // URL Filtering
  handlers->AddHandler(std::make_unique<policy::SimplePolicyHandler>(
      policy::key::kURLWhitelist, policy::policy_prefs::kUrlWhitelist,
      base::Value::Type::LIST));
  handlers->AddHandler(std::make_unique<policy::URLBlacklistPolicyHandler>());

  // HTTP Negotiate authentication
  handlers->AddHandler(std::make_unique<policy::SimplePolicyHandler>(
      policy::key::kAuthServerWhitelist, prefs::kAuthServerWhitelist,
      base::Value::Type::STRING));
  handlers->AddHandler(std::make_unique<policy::SimplePolicyHandler>(
      policy::key::kAuthAndroidNegotiateAccountType,
      prefs::kAuthAndroidNegotiateAccountType, base::Value::Type::STRING));

  return handlers;
}

}  // namespace

BisonBrowserPolicyConnector::BisonBrowserPolicyConnector()
    : BrowserPolicyConnectorBase(base::BindRepeating(&BuildHandlerList)) {}

BisonBrowserPolicyConnector::~BisonBrowserPolicyConnector() = default;

std::vector<std::unique_ptr<policy::ConfigurationPolicyProvider>>
BisonBrowserPolicyConnector::CreatePolicyProviders() {
  std::vector<std::unique_ptr<policy::ConfigurationPolicyProvider>> providers;
  providers.push_back(
      std::make_unique<policy::android::AndroidCombinedPolicyProvider>(
          GetSchemaRegistry()));
  return providers;
}

}  // namespace bison
