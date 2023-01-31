

#include "bison/browser/permission/simple_permission_request.h"

#include "bison/browser/permission/bv_permission_request.h"

#include "base/callback.h"

namespace bison {

SimplePermissionRequest::SimplePermissionRequest(const GURL& origin,
                                                 int64_t resources,
                                                 PermissionCallback callback)
    : origin_(origin), resources_(resources), callback_(std::move(callback)) {}

SimplePermissionRequest::~SimplePermissionRequest() {}

void SimplePermissionRequest::NotifyRequestResult(bool allowed) {
  std::move(callback_).Run(allowed);
}

const GURL& SimplePermissionRequest::GetOrigin() {
  return origin_;
}

int64_t SimplePermissionRequest::GetResources() {
  return resources_;
}

}  // namespace bison
