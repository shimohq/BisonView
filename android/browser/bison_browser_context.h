// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_BROWSER_BISON_BISON_ANDROID_BROWSER_CONTEXT_H_
#define BISON_ANDROID_BROWSER_BISON_BISON_ANDROID_BROWSER_CONTEXT_H_

#include <memory>
#include <vector>

#include "bison/android/browser/bison_ssl_host_state_delegate.h"
#include "bison/android/browser/network_service/bison_proxy_config_monitor.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/keyed_service/core/simple_factory_key.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/visitedlink/browser/visitedlink_delegate.h"
#include "content/public/browser/browser_context.h"

class GURL;
class PrefService;

namespace autofill {
class AutocompleteHistoryManager;
}

namespace content {
class PermissionControllerDelegate;
class ResourceContext;
class SSLHostStateDelegate;
class WebContents;
}

namespace download {
class InProgressDownloadManager;
}

namespace visitedlink {
class VisitedLinkMaster;
}

namespace bison {

class BisonFormDatabaseService;
class BisonQuotaManagerBridge;

class BisonBrowserContext : public content::BrowserContext,
                         public visitedlink::VisitedLinkDelegate {
 public:
  BisonBrowserContext();
  ~BisonBrowserContext() override;

  // Currently only one instance per process is supported.
  static BisonBrowserContext* GetDefault();

  // Convenience method to returns the BisonBrowserContext corresponding to the
  // given WebContents.
  static BisonBrowserContext* FromWebContents(
      content::WebContents* web_contents);

  base::FilePath GetCacheDir();
  base::FilePath GetPrefStorePath();
  base::FilePath GetCookieStorePath();
  static base::FilePath GetContextStoragePath();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Get the list of authentication schemes to support.
  static std::vector<std::string> GetAuthSchemes();

  // These methods map to Add methods in visitedlink::VisitedLinkMaster.
  void AddVisitedURLs(const std::vector<GURL>& urls);

  BisonQuotaManagerBridge* GetQuotaManagerBridge();
  jlong GetQuotaManagerBridge(JNIEnv* env);

  BisonFormDatabaseService* GetFormDatabaseService();
  autofill::AutocompleteHistoryManager* GetAutocompleteHistoryManager();
  CookieManager* GetCookieManager();

  // TODO(amalova): implement for non-default browser context
  bool IsDefaultBrowserContext() { return true; }

  // content::BrowserContext implementation.
  base::FilePath GetPath() override;
  bool IsOffTheRecord() override;
  content::ResourceContext* GetResourceContext() override;
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;
  storage::SpecialStoragePolicy* GetSpecialStoragePolicy() override;
  content::PushMessagingService* GetPushMessagingService() override;
  content::StorageNotificationService* GetStorageNotificationService() override;
  content::SSLHostStateDelegate* GetSSLHostStateDelegate() override;
  content::PermissionControllerDelegate* GetPermissionControllerDelegate()
      override;
  content::ClientHintsControllerDelegate* GetClientHintsControllerDelegate()
      override;
  content::BackgroundFetchDelegate* GetBackgroundFetchDelegate() override;
  content::BackgroundSyncController* GetBackgroundSyncController() override;
  content::BrowsingDataRemoverDelegate* GetBrowsingDataRemoverDelegate()
      override;
  download::InProgressDownloadManager* RetriveInProgressDownloadManager()
      override;

  // visitedlink::VisitedLinkDelegate implementation.
  void RebuildTable(const scoped_refptr<URLEnumerator>& enumerator) override;

  PrefService* GetPrefService() const { return user_pref_service_.get(); }

  void SetExtendedReportingAllowed(bool allowed);

  network::mojom::NetworkContextParamsPtr GetNetworkContextParams(
      bool in_memory,
      const base::FilePath& relative_partition_path);

  base::android::ScopedJavaLocalRef<jobject> GetJavaBrowserContext();

 private:
  void CreateUserPrefService();
  void MigrateLocalStatePrefs();

  // The file path where data for this context is persisted.
  base::FilePath context_storage_path_;

  scoped_refptr<BisonQuotaManagerBridge> quota_manager_bridge_;
  std::unique_ptr<BisonFormDatabaseService> form_database_service_;
  std::unique_ptr<autofill::AutocompleteHistoryManager>
      autocomplete_history_manager_;

  std::unique_ptr<visitedlink::VisitedLinkMaster> visitedlink_master_;
  std::unique_ptr<content::ResourceContext> resource_context_;

  std::unique_ptr<PrefService> user_pref_service_;
  std::unique_ptr<BisonSSLHostStateDelegate> ssl_host_state_delegate_;
  std::unique_ptr<content::PermissionControllerDelegate> permission_manager_;

  SimpleFactoryKey simple_factory_key_;

  base::android::ScopedJavaGlobalRef<jobject> obj_;

  DISALLOW_COPY_AND_ASSIGN(BisonBrowserContext);
};

}  // namespace bison

#endif  // BISON_ANDROID_BROWSER_BISON_BISON_ANDROID_BROWSER_CONTEXT_H_
