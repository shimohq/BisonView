// create by jiang947

#ifndef BISON_BROWSER_BISON_BROWSER_CONTEXT_H_
#define BISON_BROWSER_BISON_BROWSER_CONTEXT_H_

#include <memory>

#include "bison/browser/bv_ssl_host_state_delegate.h"
#include "bison/browser/network_service/bv_proxy_config_monitor.h"

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "components/keyed_service/core/simple_factory_key.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/visitedlink/browser/visitedlink_delegate.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/zoom_level_delegate.h"
#include "services/cert_verifier/public/mojom/cert_verifier_service_factory.mojom-forward.h"

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
class VisitedLinkWriter;
}

namespace bison {

class BvFormDatabaseService;
class BvQuotaManagerBridge;

class BvBrowserContext : public content::BrowserContext,
                            public visitedlink::VisitedLinkDelegate {
 public:
  BvBrowserContext();

  BvBrowserContext(const BvBrowserContext&) = delete;
  BvBrowserContext& operator=(const BvBrowserContext&) = delete;

  ~BvBrowserContext() override;

  // Currently only one instance per process is supported.
  static BvBrowserContext* GetDefault();

  static BvBrowserContext* FromWebContents(
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

  BvQuotaManagerBridge* GetQuotaManagerBridge();
  jlong GetQuotaManagerBridge(JNIEnv* env);

  BvFormDatabaseService* GetFormDatabaseService();
  autofill::AutocompleteHistoryManager* GetAutocompleteHistoryManager();
  CookieManager* GetCookieManager();

  // TODO(amalova): implement for non-default browser context
  bool IsDefaultBrowserContext() { return true; }

  // BrowserContext implementation.
  base::FilePath GetPath() override;
  bool IsOffTheRecord() override;
  content::ResourceContext* GetResourceContext() override;
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;
  storage::SpecialStoragePolicy* GetSpecialStoragePolicy() override;
  content::PlatformNotificationService* GetPlatformNotificationService()
      override;
  content::PushMessagingService* GetPushMessagingService() override;
  content::StorageNotificationService* GetStorageNotificationService() override;
  content::SSLHostStateDelegate* GetSSLHostStateDelegate() override;
  content::PermissionControllerDelegate* GetPermissionControllerDelegate()
      override;
  content::ClientHintsControllerDelegate* GetClientHintsControllerDelegate()
      override;
  variations::VariationsClient* GetVariationsClient() override;
  content::BackgroundFetchDelegate* GetBackgroundFetchDelegate() override;
  content::BackgroundSyncController* GetBackgroundSyncController() override;
  content::BrowsingDataRemoverDelegate* GetBrowsingDataRemoverDelegate()
      override;
  download::InProgressDownloadManager* RetriveInProgressDownloadManager()
      override;
  std::unique_ptr<content::ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath& partition_path) override;

  // visitedlink::VisitedLinkDelegate implementation.
  void RebuildTable(const scoped_refptr<URLEnumerator>& enumerator) override;

  PrefService* GetPrefService() const { return user_pref_service_.get(); }

  void SetExtendedReportingAllowed(bool allowed);

  void ConfigureNetworkContextParams(
      bool in_memory,
      const base::FilePath& relative_partition_path,
      network::mojom::NetworkContextParams* network_context_params,
      cert_verifier::mojom::CertVerifierCreationParams*
          cert_verifier_creation_params);

  base::android::ScopedJavaLocalRef<jobject> GetJavaBrowserContext();

 private:
  void CreateUserPrefService();
  void MigrateLocalStatePrefs();

  // The file path where data for this context is persisted.
  base::FilePath context_storage_path_;

  scoped_refptr<BvQuotaManagerBridge> quota_manager_bridge_;
  std::unique_ptr<BvFormDatabaseService> form_database_service_;
  std::unique_ptr<autofill::AutocompleteHistoryManager>
      autocomplete_history_manager_;

  std::unique_ptr<visitedlink::VisitedLinkWriter> visitedlink_writer_;
  std::unique_ptr<content::ResourceContext> resource_context_;

  std::unique_ptr<PrefService> user_pref_service_;
  std::unique_ptr<BvSSLHostStateDelegate> ssl_host_state_delegate_;
  std::unique_ptr<content::PermissionControllerDelegate> permission_manager_;
  std::unique_ptr<variations::VariationsClient> variations_client_;

  SimpleFactoryKey simple_factory_key_;

  base::android::ScopedJavaGlobalRef<jobject> obj_;
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_BROWSER_CONTEXT_H_
