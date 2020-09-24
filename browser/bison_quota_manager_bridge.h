// create by jiang947 


#ifndef BISON_BROWSER_BISON_QUOTA_MANAGER_BRIDGE_H_
#define BISON_BROWSER_BISON_QUOTA_MANAGER_BRIDGE_H_


#include <stdint.h>

#include <string>
#include <vector>

#include "base/android/jni_weak_ref.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"

namespace content {
class StoragePartition;
}

namespace storage {
class QuotaManager;
}  // namespace storage

namespace bison {

class BisonBrowserContext;

class BisonQuotaManagerBridge
    : public base::RefCountedThreadSafe<BisonQuotaManagerBridge> {
 public:
  static scoped_refptr<BisonQuotaManagerBridge> Create(
      BisonBrowserContext* browser_context);

  // Called by Java.
  void Init(JNIEnv* env, const base::android::JavaParamRef<jobject>& object);
  void DeleteAllData(JNIEnv* env,
                     const base::android::JavaParamRef<jobject>& object);
  void DeleteOrigin(JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& object,
                    const base::android::JavaParamRef<jstring>& origin);
  void GetOrigins(JNIEnv* env,
                  const base::android::JavaParamRef<jobject>& object,
                  jint callback_id);
  void GetUsageAndQuotaForOrigin(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& object,
      const base::android::JavaParamRef<jstring>& origin,
      jint callback_id,
      bool is_quota);

  using GetOriginsCallback =
      base::OnceCallback<void(const std::vector<std::string>& /* origin */,
                              const std::vector<int64_t>& /* usaoge */,
                              const std::vector<int64_t>& /* quota */)>;
  using QuotaUsageCallback =
      base::OnceCallback<void(int64_t /* usage */, int64_t /* quota */)>;

 private:
  friend class base::RefCountedThreadSafe<BisonQuotaManagerBridge>;
  explicit BisonQuotaManagerBridge(BisonBrowserContext* browser_context);
  ~BisonQuotaManagerBridge();

  content::StoragePartition* GetStoragePartition() const;

  storage::QuotaManager* GetQuotaManager() const;

  void DeleteAllDataOnUiThread();
  void DeleteOriginOnUiThread(const base::string16& origin);
  void GetOriginsOnUiThread(jint callback_id);
  void GetUsageAndQuotaForOriginOnUiThread(const base::string16& origin,
                                           jint callback_id,
                                           bool is_quota);

  void GetOriginsCallbackImpl(int jcallback_id,
                              const std::vector<std::string>& origin,
                              const std::vector<int64_t>& usage,
                              const std::vector<int64_t>& quota);
  void QuotaUsageCallbackImpl(int jcallback_id,
                              bool is_quota,
                              int64_t usage,
                              int64_t quota);

  BisonBrowserContext* browser_context_;
  JavaObjectWeakGlobalRef java_ref_;

  base::WeakPtrFactory<BisonQuotaManagerBridge> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(BisonQuotaManagerBridge);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_QUOTA_MANAGER_BRIDGE_H_
