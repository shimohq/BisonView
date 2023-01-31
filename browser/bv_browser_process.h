// create by jiang947

#ifndef BISON_BROWSER_BISON_BROWSER_PROCESS_H_
#define BISON_BROWSER_BISON_BROWSER_PROCESS_H_

#include "bison/browser/bv_browser_context.h"
#include "bison/browser/bv_feature_list_creator.h"
#include "bison/browser/lifecycle/bv_contents_lifecycle_notifier.h"
#include "bison/browser/bv_enterprise_authentication_app_link_manager.h"

#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/network_service_instance.h"
#include "net/log/net_log.h"
#include "services/network/network_service.h"

namespace bison {

namespace prefs {

// Used for Kerberos authentication.
extern const char kAuthAndroidNegotiateAccountType[];
extern const char kAuthServerAllowlist[];
extern const char kEnterpriseAuthAppLinkPolicy[];

}  // namespace prefs

class BvContentsLifecycleNotifier;
class VisibilityMetricsLogger;

class BvBrowserProcess {
 public:
  BvBrowserProcess(BvFeatureListCreator* bv_feature_list_creator);

  BvBrowserProcess(const BvBrowserProcess&) = delete;
  BvBrowserProcess& operator=(const BvBrowserProcess&) = delete;

  ~BvBrowserProcess();

  static BvBrowserProcess* GetInstance();

  PrefService* local_state();
  BvBrowserPolicyConnector* browser_policy_connector();
  VisibilityMetricsLogger* visibility_metrics_logger();

  void CreateBrowserPolicyConnector();
  void CreateLocalState();



  static void RegisterNetworkContextLocalStatePrefs(
      PrefRegistrySimple* pref_registry);
  static void RegisterEnterpriseAuthenticationAppLinkPolicyPref(
      PrefRegistrySimple* pref_registry);

  // Constructs HttpAuthDynamicParams based on |local_state_|.
  network::mojom::HttpAuthDynamicParamsPtr CreateHttpAuthDynamicParams();

  void PreMainMessageLoopRun();

  static void TriggerMinidumpUploading();
  EnterpriseAuthenticationAppLinkManager*
  GetEnterpriseAuthenticationAppLinkManager();

 private:
  // jiang bison not impl safe Browsing
  // void CreateSafeBrowsingUIManager();
  // void CreateSafeBrowsingWhitelistManager();

  void OnAuthPrefsChanged();

  void OnLoseForeground();

  // If non-null, this object holds a pref store that will be taken by
  // BvBrowserProcess to create the |local_state_|.
  // The BvFeatureListCreator is owned by BvMainDelegate.
  BvFeatureListCreator* bison_feature_list_creator_;

  std::unique_ptr<PrefService> local_state_;

  std::unique_ptr<BvBrowserPolicyConnector> browser_policy_connector_;

  // Accessed on both UI and IO threads.
  // jiang bison not impl safe Browsing
  // scoped_refptr<BisonSafeBrowsingUIManager> safe_browsing_ui_manager_;

  // Accessed on UI thread only.
  // jiang bison not impl safe Browsing
  // std::unique_ptr<safe_browsing::TriggerManager>
  // safe_browsing_trigger_manager_;

  // These two are accessed on IO thread only.
  // jiang bison not impl safe Browsing
  // scoped_refptr<safe_browsing::RemoteSafeBrowsingDatabaseManager>
  //     safe_browsing_db_manager_;
  // bool safe_browsing_db_manager_started_ = false;

  PrefChangeRegistrar pref_change_registrar_;

  // TODO(amalova): Consider to make WhitelistManager per-profile.
  // Accessed on UI and IO threads.
  // jiang bison not impl safe Browsing
  // std::unique_ptr<BisonSafeBrowsingWhitelistManager>
  //     safe_browsing_whitelist_manager_;

  std::unique_ptr<BvContentsLifecycleNotifier> bv_contents_lifecycle_notifier_;


  std::unique_ptr<VisibilityMetricsLogger> visibility_metrics_logger_;
  std::unique_ptr<BvContentsLifecycleNotifier> aw_contents_lifecycle_notifier_;
  std::unique_ptr<EnterpriseAuthenticationAppLinkManager> app_link_manager_;
};

}  // namespace bison
#endif  // BISON_BROWSER_BISON_BROWSER_PROCESS_H_
