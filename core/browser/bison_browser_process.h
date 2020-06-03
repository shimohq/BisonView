// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This interface is for managing the global services of the application.

#ifndef BISON_CORE_BROWSER_BISON_BROWSER_PROCESS_H_
#define BISON_CORE_BROWSER_BISON_BROWSER_PROCESS_H_

#include "bison/core/browser/bison_browser_context.h"
#include "bison/core/browser/bison_feature_list_creator.h"
#include "bison/core/browser/safe_browsing/bison_safe_browsing_ui_manager.h"
#include "bison/core/browser/safe_browsing/bison_safe_browsing_whitelist_manager.h"
#include "base/feature_list.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/android/remote_database_manager.h"
#include "components/safe_browsing/triggers/trigger_manager.h"
#include "content/public/browser/network_service_instance.h"
#include "net/log/net_log.h"
#include "services/network/network_service.h"

namespace bison {

namespace prefs {

// Used for Kerberos authentication.
extern const char kAuthAndroidNegotiateAccountType[];
extern const char kAuthServerWhitelist[];

}  // namespace prefs

class BisonBrowserProcess {
 public:
  BisonBrowserProcess(BisonFeatureListCreator* aw_feature_list_creator);
  ~BisonBrowserProcess();

  static BisonBrowserProcess* GetInstance();

  PrefService* local_state();
  BisonBrowserPolicyConnector* browser_policy_connector();

  void CreateBrowserPolicyConnector();
  void CreateLocalState();
  void InitSafeBrowsing();

  safe_browsing::RemoteSafeBrowsingDatabaseManager* GetSafeBrowsingDBManager();

  // Called on UI thread.
  // This method lazily creates TriggerManager.
  // Needs to happen after |safe_browsing_ui_manager_| is created.
  safe_browsing::TriggerManager* GetSafeBrowsingTriggerManager();

  // InitSafeBrowsing must be called first.
  // Called on UI and IO threads.
  BisonSafeBrowsingWhitelistManager* GetSafeBrowsingWhitelistManager() const;

  // InitSafeBrowsing must be called first.
  // Called on UI and IO threads.
  BisonSafeBrowsingUIManager* GetSafeBrowsingUIManager() const;

  static void RegisterNetworkContextLocalStatePrefs(
      PrefRegistrySimple* pref_registry);
  // Constructs HttpAuthDynamicParams based on |local_state_|.
  network::mojom::HttpAuthDynamicParamsPtr CreateHttpAuthDynamicParams();

  void PreMainMessageLoopRun();

 private:
  void CreateSafeBrowsingUIManager();
  void CreateSafeBrowsingWhitelistManager();

  void OnAuthPrefsChanged();

  // If non-null, this object holds a pref store that will be taken by
  // BisonBrowserProcess to create the |local_state_|.
  // The BisonFeatureListCreator is owned by BisonMainDelegate.
  BisonFeatureListCreator* aw_feature_list_creator_;

  std::unique_ptr<PrefService> local_state_;

  std::unique_ptr<BisonBrowserPolicyConnector> browser_policy_connector_;

  // Accessed on both UI and IO threads.
  scoped_refptr<BisonSafeBrowsingUIManager> safe_browsing_ui_manager_;

  // Accessed on UI thread only.
  std::unique_ptr<safe_browsing::TriggerManager> safe_browsing_trigger_manager_;

  // These two are accessed on IO thread only.
  scoped_refptr<safe_browsing::RemoteSafeBrowsingDatabaseManager>
      safe_browsing_db_manager_;
  bool safe_browsing_db_manager_started_ = false;

  PrefChangeRegistrar pref_change_registrar_;

  // TODO(amalova): Consider to make WhitelistManager per-profile.
  // Accessed on UI and IO threads.
  std::unique_ptr<BisonSafeBrowsingWhitelistManager>
      safe_browsing_whitelist_manager_;

  DISALLOW_COPY_AND_ASSIGN(BisonBrowserProcess);
};

}  // namespace bison
#endif  // BISON_CORE_BROWSER_BISON_BROWSER_PROCESS_H_
