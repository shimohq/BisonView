// create by jiang947

#ifndef BISON_BROWSER_BISON_BROWSER_CONTEXT_H_
#define BISON_BROWSER_BISON_BROWSER_CONTEXT_H_

#include <memory>

#include "bison/browser/bison_resource_context.h"
#include "bison/browser/network_service/bison_proxy_config_monitor.h"

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/keyed_service/core/simple_factory_key.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/resource_context.h"

class GURL;
class PrefService;

namespace autofill {
class AutocompleteHistoryManager;
}

namespace content {
class ContentIndexProvider;
class DownloadManagerDelegate;
class PermissionControllerDelegate;

class ZoomLevelDelegate;
class WebContents;
}  // namespace content

namespace bison {

class BisonDownloadManagerDelegate;
class BisonFormDatabaseService;
class BisonQuotaManagerBridge;

class BisonBrowserContext : public content::BrowserContext {
 public:
  BisonBrowserContext();
  ~BisonBrowserContext() override;

  static BisonBrowserContext* GetDefault();

  static BisonBrowserContext* FromWebContents(
      content::WebContents* web_contents);

  base::FilePath GetCacheDir();
  base::FilePath GetPrefStorePath();
  base::FilePath GetCookieStorePath();
  static base::FilePath GetContextStoragePath();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Get the list of authentication schemes to support.
  static std::vector<std::string> GetAuthSchemes();

  BisonQuotaManagerBridge* GetQuotaManagerBridge();
  jlong GetQuotaManagerBridge(JNIEnv* env);

  autofill::AutocompleteHistoryManager* GetAutocompleteHistoryManager();
  CookieManager* GetCookieManager();

  // TODO(amalova): implement for non-default browser context
  // jiang 这个方法可以干掉
  bool IsDefaultBrowserContext() { return true; }

  // BrowserContext implementation.
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

  network::mojom::NetworkContextParamsPtr GetNetworkContextParams(
      bool in_memory,
      const base::FilePath& relative_partition_path);

  PrefService* GetPrefService() const { return user_pref_service_.get(); }

  base::android::ScopedJavaLocalRef<jobject> GetJavaBrowserContext();

 protected:
  std::unique_ptr<BisonResourceContext> resource_context_;
  std::unique_ptr<BisonDownloadManagerDelegate> download_manager_delegate_;
  std::unique_ptr<content::PermissionControllerDelegate> permission_manager_;
  // std::unique_ptr<content::BackgroundSyncController>
  //     background_sync_controller_;
  // std::unique_ptr<ContentIndexProvider> content_index_provider_;

 private:
  void CreateUserPrefService();

  base::FilePath context_storage_path_;

  scoped_refptr<BisonQuotaManagerBridge> quota_manager_bridge_;
  std::unique_ptr<BisonFormDatabaseService> form_database_service_;
  std::unique_ptr<autofill::AutocompleteHistoryManager>
      autocomplete_history_manager_;

  std::unique_ptr<PrefService> user_pref_service_;

  SimpleFactoryKey simple_factory_key_;

  base::android::ScopedJavaGlobalRef<jobject> obj_;

  DISALLOW_COPY_AND_ASSIGN(BisonBrowserContext);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_BROWSER_CONTEXT_H_
