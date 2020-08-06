// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison_permission_manager.h"

#include "base/callback.h"
#include "base/command_line.h"
#include "content/public/browser/permission_controller.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
// #include "content/shell/common/shell_switches.h"
#include "media/base/media_switches.h"

namespace bison {

namespace {

bool IsWhitelistedPermissionType(content::PermissionType permission) {
  switch (permission) {
    case content::PermissionType::GEOLOCATION:
    case content::PermissionType::MIDI:
    case content::PermissionType::SENSORS:
    case content::PermissionType::ACCESSIBILITY_EVENTS:
    case content::PermissionType::PAYMENT_HANDLER:
    case content::PermissionType::WAKE_LOCK_SCREEN:

    // Background Sync and Background Fetch browser tests require
    // permission to be granted by default.
    case content::PermissionType::BACKGROUND_SYNC:
    case content::PermissionType::BACKGROUND_FETCH:
    case content::PermissionType::PERIODIC_BACKGROUND_SYNC:

    case content::PermissionType::IDLE_DETECTION:
      return true;
    case content::PermissionType::MIDI_SYSEX:
    case content::PermissionType::NOTIFICATIONS:
    case content::PermissionType::PROTECTED_MEDIA_IDENTIFIER:
    case content::PermissionType::DURABLE_STORAGE:
    case content::PermissionType::AUDIO_CAPTURE:
    case content::PermissionType::VIDEO_CAPTURE:
    case content::PermissionType::FLASH:
    case content::PermissionType::CLIPBOARD_READ:
    case content::PermissionType::CLIPBOARD_WRITE:
    case content::PermissionType::NUM:
    case content::PermissionType::WAKE_LOCK_SYSTEM:
      return false;
  }

  NOTREACHED();
  return false;
}

}  // namespace

BisonPermissionManager::BisonPermissionManager() = default;

BisonPermissionManager::~BisonPermissionManager() {}

int BisonPermissionManager::RequestPermission(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    base::OnceCallback<void(blink::mojom::PermissionStatus)> callback) {
  std::move(callback).Run(IsWhitelistedPermissionType(permission)
                              ? blink::mojom::PermissionStatus::GRANTED
                              : blink::mojom::PermissionStatus::DENIED);
  return content::PermissionController::kNoPendingOperation;
}

int BisonPermissionManager::RequestPermissions(
    const std::vector<content::PermissionType>& permissions,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    base::OnceCallback<void(const std::vector<blink::mojom::PermissionStatus>&)>
        callback) {
  std::vector<blink::mojom::PermissionStatus> result;
  for (const auto& permission : permissions) {
    result.push_back(IsWhitelistedPermissionType(permission)
                         ? blink::mojom::PermissionStatus::GRANTED
                         : blink::mojom::PermissionStatus::DENIED);
  }
  std::move(callback).Run(result);
  return content::PermissionController::kNoPendingOperation;
}

void BisonPermissionManager::ResetPermission(content::PermissionType permission,
                                             const GURL& requesting_origin,
                                             const GURL& embedding_origin) {}

blink::mojom::PermissionStatus BisonPermissionManager::GetPermissionStatus(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if ((permission == content::PermissionType::AUDIO_CAPTURE ||
       permission == content::PermissionType::VIDEO_CAPTURE) &&
      command_line->HasSwitch(switches::kUseFakeDeviceForMediaStream) &&
      command_line->HasSwitch(switches::kUseFakeUIForMediaStream)) {
    return blink::mojom::PermissionStatus::GRANTED;
  }

  return IsWhitelistedPermissionType(permission)
             ? blink::mojom::PermissionStatus::GRANTED
             : blink::mojom::PermissionStatus::DENIED;
}

blink::mojom::PermissionStatus
BisonPermissionManager::GetPermissionStatusForFrame(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin) {
  return GetPermissionStatus(
      permission, requesting_origin,
      content::WebContents::FromRenderFrameHost(render_frame_host)
          ->GetLastCommittedURL()
          .GetOrigin());
}

int BisonPermissionManager::SubscribePermissionStatusChange(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    base::RepeatingCallback<void(blink::mojom::PermissionStatus)> callback) {
  return content::PermissionController::kNoPendingOperation;
}

void BisonPermissionManager::UnsubscribePermissionStatusChange(
    int subscription_id) {}

}  // namespace bison
