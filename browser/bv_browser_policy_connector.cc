#include "bison/browser/bv_browser_policy_connector.h"

#include <memory>

#include "bison/browser/bv_browser_process.h"

#include "base/bind.h"
#include "components/policy/core/browser/configuration_policy_handler_list.h"
#include "components/policy/core/browser/url_blocklist_policy_handler.h"
#include "components/policy/core/common/android/android_combined_policy_provider.h"
#include "components/policy/core/common/policy_details.h"
#include "components/policy/core/common/policy_pref_names.h"
#include "components/policy/policy_constants.h"
#include "components/version_info/android/channel_getter.h"
#include "components/version_info/channel.h"
#include "net/url_request/url_request_context_getter.h"

namespace bison {

namespace {

// Factory for the handlers that will be responsible for converting the policies
// to the associated preferences.
std::unique_ptr<policy::ConfigurationPolicyHandlerList> BuildHandlerList(
    const policy::Schema& chrome_schema) {
  version_info::Channel channel = version_info::android::GetChannel();
  std::unique_ptr<policy::ConfigurationPolicyHandlerList> handlers(
      new policy::ConfigurationPolicyHandlerList(
          policy::ConfigurationPolicyHandlerList::
              PopulatePolicyHandlerParametersCallback(),
          base::BindRepeating(&policy::GetChromePolicyDetails),
          channel != version_info::Channel::STABLE &&
              channel != version_info::Channel::BETA));

  // URL Filtering
  handlers->AddHandler(std::make_unique<policy::SimplePolicyHandler>(
      policy::key::kURLAllowlist, policy::policy_prefs::kUrlAllowlist,
      base::Value::Type::LIST));
  handlers->AddHandler(std::make_unique<policy::URLBlocklistPolicyHandler>(
      policy::key::kURLBlocklist));

  // HTTP Negotiate authentication
  handlers->AddHandler(std::make_unique<policy::SimplePolicyHandler>(
      policy::key::kAuthServerAllowlist, prefs::kAuthServerAllowlist,
      base::Value::Type::STRING));
  handlers->AddHandler(std::make_unique<policy::SimplePolicyHandler>(
      policy::key::kAuthAndroidNegotiateAccountType,
      prefs::kAuthAndroidNegotiateAccountType, base::Value::Type::STRING));

  // TODO(ayushsha): Add custom SchemaValidation handler to
  // * Validate the format of url.
  // * Maximum authentication url that can be added.
  handlers->AddHandler(
      std::make_unique<policy::SimpleSchemaValidatingPolicyHandler>(
          policy::key::kEnterpriseAuthenticationAppLinkPolicy,
          prefs::kEnterpriseAuthAppLinkPolicy, chrome_schema,
          policy::SchemaOnErrorStrategy::SCHEMA_ALLOW_UNKNOWN,
          policy::SimpleSchemaValidatingPolicyHandler::RECOMMENDED_PROHIBITED,
          policy::SimpleSchemaValidatingPolicyHandler::MANDATORY_ALLOWED));

  return handlers;
}

}  // namespace

BvBrowserPolicyConnector::BvBrowserPolicyConnector()
    : BrowserPolicyConnectorBase(base::BindRepeating(&BuildHandlerList)) {}

BvBrowserPolicyConnector::~BvBrowserPolicyConnector() = default;

std::vector<std::unique_ptr<policy::ConfigurationPolicyProvider>>
BvBrowserPolicyConnector::CreatePolicyProviders() {
  std::vector<std::unique_ptr<policy::ConfigurationPolicyProvider>> providers;
  providers.push_back(
      std::make_unique<policy::android::AndroidCombinedPolicyProvider>(
          GetSchemaRegistry()));
  return providers;
}

}  // namespace bison
