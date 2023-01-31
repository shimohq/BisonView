// create by jiang947

#ifndef BISON_BROWSER_PERMISSION_MEDIA_ACCESS_PERMISSION_REQUEST_H_
#define BISON_BROWSER_PERMISSION_MEDIA_ACCESS_PERMISSION_REQUEST_H_

#include <stdint.h>

#include "base/callback.h"
#include "bison/browser/permission/bv_permission_request_delegate.h"

#include "content/public/browser/media_stream_request.h"
#include "third_party/blink/public/common/mediastream/media_stream_request.h"

namespace bison {

// The BvPermissionRequestDelegate implementation for media access permission
// request.
class MediaAccessPermissionRequest : public BvPermissionRequestDelegate {
 public:
  MediaAccessPermissionRequest(const content::MediaStreamRequest& request,
                               content::MediaResponseCallback callback);

  MediaAccessPermissionRequest(const MediaAccessPermissionRequest&) = delete;
  MediaAccessPermissionRequest& operator=(const MediaAccessPermissionRequest&) =
      delete;

  ~MediaAccessPermissionRequest() override;

  // BvPermissionRequestDelegate implementation.
  const GURL& GetOrigin() override;
  int64_t GetResources() override;
  void NotifyRequestResult(bool allowed) override;

 private:
  friend class TestMediaAccessPermissionRequest;

  const content::MediaStreamRequest request_;
  content::MediaResponseCallback callback_;

  // For test only.
  blink::MediaStreamDevices audio_test_devices_;
  blink::MediaStreamDevices video_test_devices_;
};

}  // namespace bison

#endif  // BISON_BROWSER_PERMISSION_MEDIA_ACCESS_PERMISSION_REQUEST_H_
