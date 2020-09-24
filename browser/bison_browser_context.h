// create by jiang947

#ifndef BISON_BROWSER_BISON_BROWSER_CONTEXT_H_
#define BISON_BROWSER_BISON_BROWSER_CONTEXT_H_

#include <memory>

#include "base/android/jni_weak_ref.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "bison/browser/bison_resource_context.h"
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

class BisonBrowserContext : public content::BrowserContext{
 public:
  
  BisonBrowserContext();
  ~BisonBrowserContext() override;

  static BisonBrowserContext* GetDefault();

  static BisonBrowserContext* FromWebContents(
      content::WebContents* web_contents);




  
  bool IsDefaultBrowserContext() { return true; }

  // BrowserContext implementation.
  base::FilePath GetPath() override;
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



  

  base::android::ScopedJavaLocalRef<jobject> GetJavaBrowserContext();

 protected:

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

  base::FilePath path_;

  std::unique_ptr<SimpleFactoryKey> key_;

  base::android::ScopedJavaGlobalRef<jobject> obj_;

  DISALLOW_COPY_AND_ASSIGN(BisonBrowserContext);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_BROWSER_CONTEXT_H_
