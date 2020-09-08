// create by jiang947

#ifndef BISON_BROWSER_BISON_BROWSER_CONTEXT_H_
#define BISON_BROWSER_BISON_BROWSER_CONTEXT_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/resource_context.h"

class SimpleFactoryKey;

namespace content {
class BackgroundSyncController;
class ContentIndexProvider;
class DownloadManagerDelegate;
class PermissionControllerDelegate;

class ZoomLevelDelegate;
}  // namespace content

namespace bison {

class BisonDownloadManagerDelegate;

class BisonBrowserContext : public content::BrowserContext {
 public:
  // If |delay_services_creation| is true, the owner is responsible for calling
  // CreateBrowserContextServices() for this BrowserContext.
  BisonBrowserContext(bool off_the_record,
                      bool delay_services_creation = false);
  ~BisonBrowserContext() override;

  static BisonBrowserContext* FromWebContents(
      content::WebContents* web_contents);

  void set_guest_manager_for_testing(
      content::BrowserPluginGuestManager* guest_manager) {
    guest_manager_ = guest_manager;
  }

  // BrowserContext implementation.
  base::FilePath GetPath() override;
  // #if !defined(OS_ANDROID)
  //   std::unique_ptr<ZoomLevelDelegate> CreateZoomLevelDelegate(
  //       const base::FilePath& partition_path) override;
  // #endif  // !defined(OS_ANDROID)
  bool IsOffTheRecord() override;
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::ResourceContext* GetResourceContext() override;
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
  content::ContentIndexProvider* GetContentIndexProvider() override;

 protected:
  // Contains URLRequestContextGetter required for resource loading.
  class BisonResourceContext : public content::ResourceContext {
   public:
    BisonResourceContext();
    ~BisonResourceContext() override;

   private:
    DISALLOW_COPY_AND_ASSIGN(BisonResourceContext);
  };

  bool ignore_certificate_errors() const { return ignore_certificate_errors_; }

  std::unique_ptr<BisonResourceContext> resource_context_;
  std::unique_ptr<BisonDownloadManagerDelegate> download_manager_delegate_;
  std::unique_ptr<content::PermissionControllerDelegate> permission_manager_;
  // std::unique_ptr<content::BackgroundSyncController>
  //     background_sync_controller_;
  // std::unique_ptr<ContentIndexProvider> content_index_provider_;

 private:
  // Performs initialization of the BisonBrowserContext while IO is still
  // allowed on the current thread.
  void InitWhileIOAllowed();
  void FinishInitWhileIOAllowed();

  bool ignore_certificate_errors_;
  bool off_the_record_;
  base::FilePath path_;
  content::BrowserPluginGuestManager* guest_manager_;
  std::unique_ptr<SimpleFactoryKey> key_;

  DISALLOW_COPY_AND_ASSIGN(BisonBrowserContext);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_BROWSER_CONTEXT_H_
