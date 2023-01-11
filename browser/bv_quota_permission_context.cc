
#include "bison/browser/bv_quota_permission_context.h"


using content::QuotaPermissionContext;

namespace bison {

BvQuotaPermissionContext::BvQuotaPermissionContext() {
}

BvQuotaPermissionContext::~BvQuotaPermissionContext() {
}

void BvQuotaPermissionContext::RequestQuotaPermission(
    const content::StorageQuotaParams& params,
    int render_process_id,
    PermissionCallback callback) {
  // Android WebView only uses storage::kStorageTypeTemporary type of storage
  // with quota managed automatically, not through this interface. Therefore
  // unconditionally disallow all quota requests here.
  std::move(callback).Run(QUOTA_PERMISSION_RESPONSE_DISALLOW);
}

}  // namespace android_webview
