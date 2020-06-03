// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/core/browser/bison_quota_manager_bridge.h"

#include <set>

#include "bison/core/browser/bison_browser_context.h"
#include "bison/core/browser/bison_content_browser_client.h"
#include "bison/core/browser_jni_headers/BisonQuotaManagerBridge_jni.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/scoped_refptr.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_client.h"
#include "storage/browser/quota/quota_manager.h"
#include "third_party/blink/public/mojom/quota/quota_types.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

using base::android::AttachCurrentThread;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;
using content::StoragePartition;
using storage::QuotaClient;
using storage::QuotaManager;

namespace bison {

namespace {

// This object lives on UI and IO threads. Care need to be taken to make sure
// there are no concurrent accesses to instance variables. Also this object
// is refcounted in the various callbacks, and is destroyed when all callbacks
// are destroyed at the end of DoneOnUIThread.
class GetOriginsTask : public base::RefCountedThreadSafe<GetOriginsTask> {
 public:
  GetOriginsTask(BisonQuotaManagerBridge::GetOriginsCallback callback,
                 QuotaManager* quota_manager);

  void Run();

 private:
  friend class base::RefCountedThreadSafe<GetOriginsTask>;
  ~GetOriginsTask();

  void OnOriginsObtained(const std::set<url::Origin>& origins,
                         blink::mojom::StorageType type);

  void OnUsageAndQuotaObtained(const url::Origin& origin,
                               blink::mojom::QuotaStatusCode status_code,
                               int64_t usage,
                               int64_t quota);

  void CheckDone();
  void DoneOnUIThread();

  BisonQuotaManagerBridge::GetOriginsCallback ui_callback_;
  scoped_refptr<QuotaManager> quota_manager_;

  std::vector<std::string> origin_;
  std::vector<int64_t> usage_;
  std::vector<int64_t> quota_;

  size_t num_callbacks_to_wait_;
  size_t num_callbacks_received_;

  DISALLOW_COPY_AND_ASSIGN(GetOriginsTask);
};

GetOriginsTask::GetOriginsTask(
    BisonQuotaManagerBridge::GetOriginsCallback callback,
    QuotaManager* quota_manager)
    : ui_callback_(std::move(callback)), quota_manager_(quota_manager) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

GetOriginsTask::~GetOriginsTask() {}

void GetOriginsTask::Run() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  base::PostTask(
      FROM_HERE, {BrowserThread::IO},
      base::BindOnce(&QuotaManager::GetOriginsModifiedSince, quota_manager_,
                     blink::mojom::StorageType::kTemporary,
                     base::Time() /* Since beginning of time. */,
                     base::BindOnce(&GetOriginsTask::OnOriginsObtained, this)));
}

void GetOriginsTask::OnOriginsObtained(const std::set<url::Origin>& origins,
                                       blink::mojom::StorageType type) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  num_callbacks_to_wait_ = origins.size();
  num_callbacks_received_ = 0u;

  for (const url::Origin& origin : origins) {
    quota_manager_->GetUsageAndQuota(
        origin, type,
        base::BindOnce(&GetOriginsTask::OnUsageAndQuotaObtained, this, origin));
  }

  CheckDone();
}

void GetOriginsTask::OnUsageAndQuotaObtained(
    const url::Origin& origin,
    blink::mojom::QuotaStatusCode status_code,
    int64_t usage,
    int64_t quota) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (status_code == blink::mojom::QuotaStatusCode::kOk) {
    origin_.push_back(origin.GetURL().spec());
    usage_.push_back(usage);
    quota_.push_back(quota);
  }

  ++num_callbacks_received_;
  CheckDone();
}

void GetOriginsTask::CheckDone() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (num_callbacks_received_ == num_callbacks_to_wait_) {
    base::PostTask(FROM_HERE, {BrowserThread::UI},
                   base::BindOnce(&GetOriginsTask::DoneOnUIThread, this));
  } else if (num_callbacks_received_ > num_callbacks_to_wait_) {
    NOTREACHED();
  }
}

// This method is to avoid copying the 3 vector arguments into a bound callback.
void GetOriginsTask::DoneOnUIThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  std::move(ui_callback_).Run(origin_, usage_, quota_);
}

void RunOnUIThread(base::OnceClosure task) {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    std::move(task).Run();
  } else {
    base::PostTask(FROM_HERE, {BrowserThread::UI}, std::move(task));
  }
}

}  // namespace

// static
jlong JNI_BisonQuotaManagerBridge_GetDefaultNativeBisonQuotaManagerBridge(
    JNIEnv* env) {
  BisonBrowserContext* browser_context = BisonBrowserContext::GetDefault();

  BisonQuotaManagerBridge* bridge = static_cast<BisonQuotaManagerBridge*>(
      browser_context->GetQuotaManagerBridge());
  DCHECK(bridge);
  return reinterpret_cast<intptr_t>(bridge);
}

// static
scoped_refptr<BisonQuotaManagerBridge> BisonQuotaManagerBridge::Create(
    BisonBrowserContext* browser_context) {
  return new BisonQuotaManagerBridge(browser_context);
}

BisonQuotaManagerBridge::BisonQuotaManagerBridge(BisonBrowserContext* browser_context)
    : browser_context_(browser_context) {}

BisonQuotaManagerBridge::~BisonQuotaManagerBridge() {}

void BisonQuotaManagerBridge::Init(JNIEnv* env,
                                const JavaParamRef<jobject>& object) {
  java_ref_ = JavaObjectWeakGlobalRef(env, object);
}

StoragePartition* BisonQuotaManagerBridge::GetStoragePartition() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // AndroidWebview does not use per-site storage partitions.
  StoragePartition* storage_partition =
      content::BrowserContext::GetDefaultStoragePartition(browser_context_);
  DCHECK(storage_partition);
  return storage_partition;
}

QuotaManager* BisonQuotaManagerBridge::GetQuotaManager() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  QuotaManager* quota_manager = GetStoragePartition()->GetQuotaManager();
  DCHECK(quota_manager);
  return quota_manager;
}

void BisonQuotaManagerBridge::DeleteAllData(JNIEnv* env,
                                         const JavaParamRef<jobject>& object) {
  RunOnUIThread(
      base::BindOnce(&BisonQuotaManagerBridge::DeleteAllDataOnUiThread, this));
}

void BisonQuotaManagerBridge::DeleteAllDataOnUiThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  GetStoragePartition()->ClearData(
      // Clear all web storage data except cookies.
      StoragePartition::REMOVE_DATA_MASK_APPCACHE |
          StoragePartition::REMOVE_DATA_MASK_FILE_SYSTEMS |
          StoragePartition::REMOVE_DATA_MASK_INDEXEDDB |
          StoragePartition::REMOVE_DATA_MASK_LOCAL_STORAGE |
          StoragePartition::REMOVE_DATA_MASK_WEBSQL,
      StoragePartition::QUOTA_MANAGED_STORAGE_MASK_TEMPORARY, GURL(),
      base::Time(), base::Time::Max(), base::DoNothing());
}

void BisonQuotaManagerBridge::DeleteOrigin(JNIEnv* env,
                                        const JavaParamRef<jobject>& object,
                                        const JavaParamRef<jstring>& origin) {
  base::string16 origin_string(
      base::android::ConvertJavaStringToUTF16(env, origin));
  RunOnUIThread(base::BindOnce(&BisonQuotaManagerBridge::DeleteOriginOnUiThread,
                               this, origin_string));
}

void BisonQuotaManagerBridge::DeleteOriginOnUiThread(
    const base::string16& origin) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  StoragePartition* storage_partition = GetStoragePartition();
  storage_partition->ClearDataForOrigin(
      // All (temporary) QuotaClient types.
      StoragePartition::REMOVE_DATA_MASK_APPCACHE |
          StoragePartition::REMOVE_DATA_MASK_FILE_SYSTEMS |
          StoragePartition::REMOVE_DATA_MASK_INDEXEDDB |
          StoragePartition::REMOVE_DATA_MASK_WEBSQL,
      StoragePartition::QUOTA_MANAGED_STORAGE_MASK_TEMPORARY, GURL(origin));
}

void BisonQuotaManagerBridge::GetOrigins(JNIEnv* env,
                                      const JavaParamRef<jobject>& object,
                                      jint callback_id) {
  RunOnUIThread(base::BindOnce(&BisonQuotaManagerBridge::GetOriginsOnUiThread,
                               this, callback_id));
}

void BisonQuotaManagerBridge::GetOriginsOnUiThread(jint callback_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  GetOriginsCallback ui_callback =
      base::BindOnce(&BisonQuotaManagerBridge::GetOriginsCallbackImpl,
                     weak_factory_.GetWeakPtr(), callback_id);
  base::MakeRefCounted<GetOriginsTask>(std::move(ui_callback),
                                       GetQuotaManager())
      ->Run();
}

void BisonQuotaManagerBridge::GetOriginsCallbackImpl(
    int jcallback_id,
    const std::vector<std::string>& origin,
    const std::vector<int64_t>& usage,
    const std::vector<int64_t>& quota) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;

  Java_BisonQuotaManagerBridge_onGetOriginsCallback(
      env, obj, jcallback_id, base::android::ToJavaArrayOfStrings(env, origin),
      base::android::ToJavaLongArray(env, usage),
      base::android::ToJavaLongArray(env, quota));
}

namespace {

void OnUsageAndQuotaObtained(
    BisonQuotaManagerBridge::QuotaUsageCallback ui_callback,
    blink::mojom::QuotaStatusCode status_code,
    int64_t usage,
    int64_t quota) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (status_code != blink::mojom::QuotaStatusCode::kOk) {
    usage = 0;
    quota = 0;
  }
  base::PostTask(FROM_HERE, {BrowserThread::UI},
                 base::BindOnce(std::move(ui_callback), usage, quota));
}

}  // namespace

void BisonQuotaManagerBridge::GetUsageAndQuotaForOrigin(
    JNIEnv* env,
    const JavaParamRef<jobject>& object,
    const JavaParamRef<jstring>& origin,
    jint callback_id,
    bool is_quota) {
  base::string16 origin_string(
      base::android::ConvertJavaStringToUTF16(env, origin));
  RunOnUIThread(
      base::BindOnce(&BisonQuotaManagerBridge::GetUsageAndQuotaForOriginOnUiThread,
                     this, origin_string, callback_id, is_quota));
}

void BisonQuotaManagerBridge::GetUsageAndQuotaForOriginOnUiThread(
    const base::string16& origin,
    jint callback_id,
    bool is_quota) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  QuotaUsageCallback ui_callback =
      base::BindOnce(&BisonQuotaManagerBridge::QuotaUsageCallbackImpl,
                     weak_factory_.GetWeakPtr(), callback_id, is_quota);

  // TODO(crbug.com/889590): Use helper for url::Origin creation from string.
  base::PostTask(
      FROM_HERE, {BrowserThread::IO},
      base::BindOnce(
          &QuotaManager::GetUsageAndQuota, GetQuotaManager(),
          url::Origin::Create(GURL(origin)),
          blink::mojom::StorageType::kTemporary,
          base::BindOnce(&OnUsageAndQuotaObtained, std::move(ui_callback))));
}

void BisonQuotaManagerBridge::QuotaUsageCallbackImpl(int jcallback_id,
                                                  bool is_quota,
                                                  int64_t usage,
                                                  int64_t quota) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;

  Java_BisonQuotaManagerBridge_onGetUsageAndQuotaForOriginCallback(
      env, obj, jcallback_id, is_quota, usage, quota);
}

}  // namespace bison
