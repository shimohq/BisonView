// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison_browser_context.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/threading/thread.h"
#include "bison_download_manager_delegate.h"
#include "bison_permission_manager.h"
#include "build/build_config.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/simple_dependency_manager.h"
#include "components/keyed_service/core/simple_factory_key.h"
#include "components/keyed_service/core/simple_key_map.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_switches.h"

#if defined(OS_WIN)
#include "base/base_paths_win.h"
#elif defined(OS_LINUX)
#include "base/nix/xdg_util.h"
#elif defined(OS_MACOSX)
#include "base/base_paths_mac.h"
#elif defined(OS_FUCHSIA)
#include "base/base_paths_fuchsia.h"
#endif

using content::BackgroundFetchDelegate;
using content::BackgroundSyncController;
using content::BrowserPluginGuestManager;
using content::BrowserThread;
using content::BrowsingDataRemoverDelegate;
using content::ClientHintsControllerDelegate;
using content::ContentIndexProvider;
using content::DownloadManagerDelegate;
using content::PermissionControllerDelegate;
using content::PushMessagingService;
using content::ResourceContext;
using content::SSLHostStateDelegate;
using content::StorageNotificationService;

namespace bison {

BisonBrowserContext::BisonResourceContext::BisonResourceContext() {}

BisonBrowserContext::BisonResourceContext::~BisonResourceContext() {}

BisonBrowserContext* BisonBrowserContext::FromWebContents(
    content::WebContents* web_contents) {
  return static_cast<BisonBrowserContext*>(web_contents->GetBrowserContext());
}

BisonBrowserContext::BisonBrowserContext(bool off_the_record,
                                         bool delay_services_creation)
    : resource_context_(new BisonResourceContext),
      ignore_certificate_errors_(false),
      off_the_record_(off_the_record),
      guest_manager_(nullptr) {
  InitWhileIOAllowed();
  VLOG(0) << "new BisonBrowserContext delay_services_creation:"
          << delay_services_creation;
  if (!delay_services_creation) {
    BrowserContextDependencyManager::GetInstance()
        ->CreateBrowserContextServices(this);
  }
}

BisonBrowserContext::~BisonBrowserContext() {
  NotifyWillBeDestroyed(this);

  // The SimpleDependencyManager should always be passed after the
  // BrowserContextDependencyManager. This is because the KeyedService instances
  // in the BrowserContextDependencyManager's dependency graph can depend on the
  // ones in the SimpleDependencyManager's graph.
  // deps //components/keyed_service/content
  DependencyManager::PerformInterlockedTwoPhaseShutdown(
      BrowserContextDependencyManager::GetInstance(), this,
      SimpleDependencyManager::GetInstance(), key_.get());

  SimpleKeyMap::GetInstance()->Dissociate(this);

  // Need to destruct the ResourceContext before posting tasks which may delete
  // the URLRequestContext because ResourceContext's destructor will remove any
  // outstanding request while URLRequestContext's destructor ensures that there
  // are no more outstanding requests.
  if (resource_context_) {
    BrowserThread::DeleteSoon(BrowserThread::IO, FROM_HERE,
                              resource_context_.release());
  }
  ShutdownStoragePartitions();
}

void BisonBrowserContext::InitWhileIOAllowed() {
  CHECK(base::PathService::Get(base::DIR_ANDROID_APP_DATA, &path_));
  path_ = path_.Append(FILE_PATH_LITERAL("bison"));

  if (!base::PathExists(path_))
    base::CreateDirectory(path_);
  VLOG(0) << "path_:" << path_.value();
  FinishInitWhileIOAllowed();
}

void BisonBrowserContext::FinishInitWhileIOAllowed() {
  BrowserContext::Initialize(this, path_);
  key_ = std::make_unique<SimpleFactoryKey>(path_, off_the_record_);
  SimpleKeyMap::GetInstance()->Associate(this, key_.get());
}

// #if !defined(OS_ANDROID)
// std::unique_ptr<ZoomLevelDelegate>
// BisonBrowserContext::CreateZoomLevelDelegate(
//     const base::FilePath&) {
//   return std::unique_ptr<ZoomLevelDelegate>();
// }
// #endif  // !defined(OS_ANDROID)

base::FilePath BisonBrowserContext::GetPath() {
  return path_;
}

bool BisonBrowserContext::IsOffTheRecord() {
  return off_the_record_;
}

DownloadManagerDelegate* BisonBrowserContext::GetDownloadManagerDelegate() {
  if (!download_manager_delegate_.get()) {
    download_manager_delegate_.reset(new BisonDownloadManagerDelegate());
    download_manager_delegate_->SetDownloadManager(
        BrowserContext::GetDownloadManager(this));
  }

  return download_manager_delegate_.get();
}

ResourceContext* BisonBrowserContext::GetResourceContext() {
  return resource_context_.get();
}

BrowserPluginGuestManager* BisonBrowserContext::GetGuestManager() {
  return guest_manager_;
}

storage::SpecialStoragePolicy* BisonBrowserContext::GetSpecialStoragePolicy() {
  return nullptr;
}

PushMessagingService* BisonBrowserContext::GetPushMessagingService() {
  return nullptr;
}

StorageNotificationService*
BisonBrowserContext::GetStorageNotificationService() {
  return nullptr;
}

SSLHostStateDelegate* BisonBrowserContext::GetSSLHostStateDelegate() {
  return nullptr;
}

PermissionControllerDelegate*
BisonBrowserContext::GetPermissionControllerDelegate() {
  if (!permission_manager_.get())
    permission_manager_.reset(new BisonPermissionManager());
  return permission_manager_.get();
}

ClientHintsControllerDelegate*
BisonBrowserContext::GetClientHintsControllerDelegate() {
  return nullptr;
}

BackgroundFetchDelegate* BisonBrowserContext::GetBackgroundFetchDelegate() {
  return nullptr;
}

content::BackgroundSyncController*
BisonBrowserContext::GetBackgroundSyncController() {
  //   // if (!background_sync_controller_)
  //   // background_sync_controller_.reset(new MockBackgroundSyncController());
  //   // background_sync_controller_.reset(new BackgroundSyncController());

  //   // return background_sync_controller_.get();
  //   VLOG(0) << "jiang GetBackgroundSyncController null";
  return nullptr;
}

BrowsingDataRemoverDelegate*
BisonBrowserContext::GetBrowsingDataRemoverDelegate() {
  return nullptr;
}

ContentIndexProvider* BisonBrowserContext::GetContentIndexProvider() {
  // if (!content_index_provider_)
  //   content_index_provider_ =
  //   std::make_unique<WebTestContentIndexProvider>();
  // return content_index_provider_.get();
  VLOG(0) << "jiang GetContentIndexProvider null";
  return nullptr;
}

}  // namespace bison
