// create by jiang947

#ifndef BISON_BROWSER_BISON_PERMISSION_MANAGER_H_
#define BISON_BROWSER_BISON_PERMISSION_MANAGER_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "content/public/browser/permission_controller_delegate.h"

namespace bison {

class BisonPermissionManager : public content::PermissionControllerDelegate {
 public:
  BisonPermissionManager();
  ~BisonPermissionManager() override;

  // PermissionManager implementation.
  int RequestPermission(content::PermissionType permission,
                        content::RenderFrameHost* render_frame_host,
                        const GURL& requesting_origin,
                        bool user_gesture,
                        base::OnceCallback<void(blink::mojom::PermissionStatus)>
                            callback) override;
  int RequestPermissions(
      const std::vector<content::PermissionType>& permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      base::OnceCallback<
          void(const std::vector<blink::mojom::PermissionStatus>&)> callback)
      override;
  void ResetPermission(content::PermissionType permission,
                       const GURL& requesting_origin,
                       const GURL& embedding_origin) override;
  blink::mojom::PermissionStatus GetPermissionStatus(
      content::PermissionType permission,
      const GURL& requesting_origin,
      const GURL& embedding_origin) override;
  blink::mojom::PermissionStatus GetPermissionStatusForFrame(
      content::PermissionType permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin) override;
  int SubscribePermissionStatusChange(
      content::PermissionType permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      base::RepeatingCallback<void(blink::mojom::PermissionStatus)> callback)
      override;
  void UnsubscribePermissionStatusChange(int subscription_id) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BisonPermissionManager);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_PERMISSION_MANAGER_H_
