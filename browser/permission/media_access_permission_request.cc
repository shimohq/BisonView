#include "bison/browser/permission/media_access_permission_request.h"

#include <utility>

#include "bison/browser/permission/bv_permission_request.h"
#include "content/public/browser/media_capture_devices.h"
#include "third_party/blink/public/mojom/mediastream/media_stream.mojom-shared.h"
#include "third_party/blink/public/mojom/mediastream/media_stream.mojom.h"

using blink::MediaStreamDevice;
using blink::MediaStreamDevices;
using content::MediaCaptureDevices;

namespace bison {

namespace {

// Return the device specified by |device_id| if exists, otherwise the first
// available device is returned.
const MediaStreamDevice* GetDeviceByIdOrFirstAvailable(
    const MediaStreamDevices& devices,
    const std::string& device_id) {
  if (devices.empty())
    return NULL;

  if (!device_id.empty()) {
    for (size_t i = 0; i < devices.size(); ++i) {
      if (devices[i].id == device_id)
        return &devices[i];
    }
  }

  return &devices[0];
}

}  // namespace

MediaAccessPermissionRequest::MediaAccessPermissionRequest(
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback callback)
    : request_(request), callback_(std::move(callback)) {}

MediaAccessPermissionRequest::~MediaAccessPermissionRequest() {}

void MediaAccessPermissionRequest::NotifyRequestResult(bool allowed) {
  std::unique_ptr<content::MediaStreamUI> ui;
  blink::mojom::StreamDevices devices;
  if (!allowed) {
    std::move(callback_).Run(
        devices, blink::mojom::MediaStreamRequestResult::PERMISSION_DENIED,
        std::move(ui));
    return;
  }

  if (request_.audio_type ==
      blink::mojom::MediaStreamType::DEVICE_AUDIO_CAPTURE) {
    const MediaStreamDevices& audio_devices =
        audio_test_devices_.empty()
            ? MediaCaptureDevices::GetInstance()->GetAudioCaptureDevices()
            : audio_test_devices_;
    const MediaStreamDevice* device = GetDeviceByIdOrFirstAvailable(
        audio_devices, request_.requested_audio_device_id);
    if (device)
      devices.audio_device = *device;
  }

  if (request_.video_type ==
      blink::mojom::MediaStreamType::DEVICE_VIDEO_CAPTURE) {
    const MediaStreamDevices& video_devices =
        video_test_devices_.empty()
            ? MediaCaptureDevices::GetInstance()->GetVideoCaptureDevices()
            : video_test_devices_;
    const MediaStreamDevice* device = GetDeviceByIdOrFirstAvailable(
        video_devices, request_.requested_video_device_id);
    if (device)
      devices.video_device = *device;
  }
  std::move(callback_).Run(
      devices,
      (!devices.audio_device.has_value() && !devices.video_device.has_value())
          ? blink::mojom::MediaStreamRequestResult::NO_HARDWARE
                      : blink::mojom::MediaStreamRequestResult::OK,
      std::move(ui));
}

const GURL& MediaAccessPermissionRequest::GetOrigin() {
  return request_.security_origin;
}

int64_t MediaAccessPermissionRequest::GetResources() {
  return (request_.audio_type ==
                  blink::mojom::MediaStreamType::DEVICE_AUDIO_CAPTURE
              ? BvPermissionRequest::AudioCapture
              : 0) |
         (request_.video_type ==
                  blink::mojom::MediaStreamType::DEVICE_VIDEO_CAPTURE
              ? BvPermissionRequest::VideoCapture
              : 0);
}

}  // namespace bison
