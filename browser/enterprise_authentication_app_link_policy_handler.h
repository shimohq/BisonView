#ifndef BISON_BROWSER_ENTERPRISE_AUTHENTICATION_APP_LINK_POLICY_HANDLER_H_
#define BISON_BROWSER_ENTERPRISE_AUTHENTICATION_APP_LINK_POLICY_HANDLER_H_


#include "bison/browser/bv_browser_process.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/core/browser/policy_error_map.h"
#include "components/policy/core/browser/url_allowlist_policy_handler.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "components/url_matcher/url_util.h"

namespace policy {

// Policy handler for EnterpriseAuthenticationAppLink policy
class EnterpriseAuthenticationAppLinkPolicyHandler
    : public TypeCheckingPolicyHandler {
 public:
  EnterpriseAuthenticationAppLinkPolicyHandler(const char* policy_name,
                                               const char* pref_path);

  EnterpriseAuthenticationAppLinkPolicyHandler(
      const EnterpriseAuthenticationAppLinkPolicyHandler&) = delete;
  EnterpriseAuthenticationAppLinkPolicyHandler& operator=(
      const EnterpriseAuthenticationAppLinkPolicyHandler&) = delete;
  ~EnterpriseAuthenticationAppLinkPolicyHandler() override;

  // ConfigurationPolicyHandler methods:
  bool CheckPolicySettings(const PolicyMap& policies,
                           PolicyErrorMap* errors) override;
  void ApplyPolicySettings(const PolicyMap& policies,
                           PrefValueMap* prefs) override;

 private:
  bool ValidatePolicyEntry(const std::string* policy);
  const char* pref_path_;
};

}  // namespace policy

#endif  // BISON_BROWSER_ENTERPRISE_AUTHENTICATION_APP_LINK_POLICY_HANDLER_H_
