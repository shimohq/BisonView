// create by jiang947 


#ifndef BISON_BROWSER_BISON_QUOTA_PERMISSION_CONTEXT_H_
#define BISON_BROWSER_BISON_QUOTA_PERMISSION_CONTEXT_H_


#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/quota_permission_context.h"

namespace bison {

class BisonQuotaPermissionContext : public content::QuotaPermissionContext {
 public:
  BisonQuotaPermissionContext();

  void RequestQuotaPermission(const content::StorageQuotaParams& params,
                              int render_process_id,
                              PermissionCallback callback) override;

 private:
  ~BisonQuotaPermissionContext() override;

  DISALLOW_COPY_AND_ASSIGN(BisonQuotaPermissionContext);
};

}  // namespace android_webview

#endif  // BISON_BROWSER_BISON_QUOTA_PERMISSION_CONTEXT_H_

