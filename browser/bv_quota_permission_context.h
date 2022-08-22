#ifndef BISON_BROWSER_BV_QUOTA_PERMISSION_CONTEXT_H_
#define BISON_BROWSER_BV_QUOTA_PERMISSION_CONTEXT_H_

#include "base/compiler_specific.h"
#include "content/public/browser/quota_permission_context.h"

namespace bison {

class BvQuotaPermissionContext : public content::QuotaPermissionContext {
 public:
  BvQuotaPermissionContext();

  BvQuotaPermissionContext(const BvQuotaPermissionContext&) = delete;
  BvQuotaPermissionContext& operator=(const BvQuotaPermissionContext&) = delete;

  void RequestQuotaPermission(const content::StorageQuotaParams& params,
                              int render_process_id,
                              PermissionCallback callback) override;

 private:
  ~BvQuotaPermissionContext() override;
};

}  // namespace android_webview

#endif  // BISON_BROWSER_BV_QUOTA_PERMISSION_CONTEXT_H_
