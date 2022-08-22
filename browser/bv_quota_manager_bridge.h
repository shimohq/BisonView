// create by jiang947

#ifndef BISON_BROWSER_BISON_QUOTA_MANAGER_BRIDGE_H_
#define BISON_BROWSER_BISON_QUOTA_MANAGER_BRIDGE_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/android/jni_weak_ref.h"
#include "base/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"

namespace content {
class StoragePartition;
}

namespace storage {
class QuotaManager;
}  // namespace storage

namespace bison {

class BvBrowserContext;

class BvQuotaManagerBridge
    : public base::RefCountedThreadSafe<BvQuotaManagerBridge> {
 public:
  BvQuotaManagerBridge(const BvQuotaManagerBridge&) = delete;
  BvQuotaManagerBridge& operator=(const BvQuotaManagerBridge&) = delete;

  static scoped_refptr<BvQuotaManagerBridge> Create(
      BvBrowserContext* browser_context);

  // Called by Java.
  void Init(JNIEnv* env, const base::android::JavaParamRef<jobject>& object);
  void DeleteAllData(JNIEnv* env,
                     const base::android::JavaParamRef<jobject>& object);
  void DeleteOrigin(JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& object,
                    const base::android::JavaParamRef<jstring>& origin);
  void GetOrigins(JNIEnv* env,
                  const base::android::JavaParamRef<jobject>& object,
                  const base::android::JavaParamRef<jobject>& callback);
  void GetUsageAndQuotaForOrigin(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& object,
      const base::android::JavaParamRef<jstring>& origin,
      const base::android::JavaParamRef<jobject>& callback,
      bool is_quota);

  using GetOriginsCallback =
      base::OnceCallback<void(const std::vector<std::string>& /* origin */,
                              const std::vector<int64_t>& /* usaoge */,
                              const std::vector<int64_t>& /* quota */)>;
  using QuotaUsageCallback =
      base::OnceCallback<void(int64_t /* usage */, int64_t /* quota */)>;

 private:
  friend class base::RefCountedThreadSafe<BvQuotaManagerBridge>;
  explicit BvQuotaManagerBridge(BvBrowserContext* browser_context);
  ~BvQuotaManagerBridge();

  content::StoragePartition* GetStoragePartition() const;

  storage::QuotaManager* GetQuotaManager() const;

  raw_ptr<BvBrowserContext> browser_context_;
  JavaObjectWeakGlobalRef java_ref_;

  base::WeakPtrFactory<BvQuotaManagerBridge> weak_factory_{this};


};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_QUOTA_MANAGER_BRIDGE_H_
