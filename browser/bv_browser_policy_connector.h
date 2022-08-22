// create by jiang947

#ifndef BISON_BROWSER_BISON_BROWSER_POLICY_CONNECTOR_H_
#define BISON_BROWSER_BISON_BROWSER_POLICY_CONNECTOR_H_

#include "components/policy/core/browser/browser_policy_connector_base.h"

namespace bison {

// Sets up and keeps the browser-global policy objects such as the PolicyService
// and the platform-specific PolicyProvider.
class BvBrowserPolicyConnector : public policy::BrowserPolicyConnectorBase {
 public:
  BvBrowserPolicyConnector();

  BvBrowserPolicyConnector(const BvBrowserPolicyConnector&) = delete;
  BvBrowserPolicyConnector& operator=(const BvBrowserPolicyConnector&) = delete;

  ~BvBrowserPolicyConnector() override;

 protected:
  // policy::BrowserPolicyConnectorBase:
  std::vector<std::unique_ptr<policy::ConfigurationPolicyProvider>>
  CreatePolicyProviders() override;
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_BROWSER_POLICY_CONNECTOR_H_
