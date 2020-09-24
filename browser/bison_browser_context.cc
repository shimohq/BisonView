// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison_browser_context.h"

#include <utility>

#include "bison/bison_jni_headers/BisonBrowserContext_jni.h"
#include "bison/browser/bison_download_manager_delegate.h"
#include "bison/browser/bison_permission_manager.h"
#include "bison/browser/bison_resource_context.h"
#include "bison/browser/cookie_manager.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/threading/thread.h"

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

using base::FilePath;
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

namespace {
BisonBrowserContext* g_browser_context = NULL;
}  // namespace

// static
BisonBrowserContext* BisonBrowserContext::GetDefault() {
  // TODO(joth): rather than store in a global here, lookup this instance
  // from the Java-side peer.
  return g_browser_context;
}

BisonBrowserContext* BisonBrowserContext::FromWebContents(
    content::WebContents* web_contents) {
  return static_cast<BisonBrowserContext*>(web_contents->GetBrowserContext());
}

base::FilePath BisonBrowserContext::GetCacheDir() {
  FilePath cache_path;
  if (!base::PathService::Get(base::DIR_CACHE, &cache_path)) {
    NOTREACHED() << "Failed to get app cache directory for Android WebView";
  }
  cache_path = cache_path.Append(FILE_PATH_LITERAL("Default"))
                   .Append(FILE_PATH_LITERAL("HTTP Cache"));
  return cache_path;
}

base::FilePath BisonBrowserContext::GetPrefStorePath() {
  FilePath pref_store_path;
  base::PathService::Get(base::DIR_ANDROID_APP_DATA, &pref_store_path);
  // TODO(amalova): Assign a proper file path for non-default profiles
  // when we support multiple profiles
  pref_store_path =
      pref_store_path.Append(FILE_PATH_LITERAL("Default/Preferences"));

  return pref_store_path;
}

base::FilePath BisonBrowserContext::GetCookieStorePath() {
  return GetCookieManager()->GetCookieStorePath();
}

// static
base::FilePath BisonBrowserContext::GetContextStoragePath() {
  base::FilePath user_data_dir;
  if (!base::PathService::Get(base::DIR_ANDROID_APP_DATA, &user_data_dir)) {
    NOTREACHED() << "Failed to get app data directory for Android WebView";
  }

  user_data_dir = user_data_dir.Append(FILE_PATH_LITERAL("Default"));
  return user_data_dir;
}

BisonBrowserContext::BisonBrowserContext() {
  DCHECK(!g_browser_context);
  g_browser_context = this;
  InitWhileIOAllowed();
}

BisonBrowserContext::~BisonBrowserContext() {
  DCHECK_EQ(this, g_browser_context);
  NotifyWillBeDestroyed(this);
  g_browser_context = NULL;
  
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
  key_ = std::make_unique<SimpleFactoryKey>(path_, IsOffTheRecord());
  SimpleKeyMap::GetInstance()->Associate(this, key_.get());
}

// #if !defined(OS_ANDROID)
// std::unique_ptr<ZoomLevelDelegate>
// BisonBrowserContext::CreateZoomLevelDelegate(
//     const base::FilePath&) {
//   return std::unique_ptr<ZoomLevelDelegate>();
// }
// #endif  // !defined(OS_ANDROID)
CookieManager* BisonBrowserContext::GetCookieManager() {
  // TODO(amalova): create cookie manager for non-default profile
  return CookieManager::GetInstance();
}

base::FilePath BisonBrowserContext::GetPath() {
  return path_;
}

bool BisonBrowserContext::IsOffTheRecord() {
  return false;
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
  if (!resource_context_) {
    resource_context_.reset(new BisonResourceContext);
  }
  return resource_context_.get();
}

BrowserPluginGuestManager* BisonBrowserContext::GetGuestManager() {
  return NULL;
}

storage::SpecialStoragePolicy* BisonBrowserContext::GetSpecialStoragePolicy() {
  return NULL;
}

PushMessagingService* BisonBrowserContext::GetPushMessagingService() {
  return NULL;
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

base::android::ScopedJavaLocalRef<jobject>
JNI_BisonBrowserContext_GetDefaultJava(JNIEnv* env) {
  return g_browser_context->GetJavaBrowserContext();
}

base::android::ScopedJavaLocalRef<jobject>
BisonBrowserContext::GetJavaBrowserContext() {
  if (!obj_) {
    JNIEnv* env = base::android::AttachCurrentThread();
    obj_ = Java_BisonBrowserContext_create(
        env, reinterpret_cast<intptr_t>(this), IsDefaultBrowserContext());
  }
  return base::android::ScopedJavaLocalRef<jobject>(obj_);
}

}  // namespace bison
