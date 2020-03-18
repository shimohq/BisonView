// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/browser/bison_javascript_dialog_manager.h"

#include <utility>

#include "bison/android/browser/bison_contents_client_bridge.h"
#include "content/public/browser/javascript_dialog_manager.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace bison {

BisonJavaScriptDialogManager::BisonJavaScriptDialogManager() {}

BisonJavaScriptDialogManager::~BisonJavaScriptDialogManager() {}

void BisonJavaScriptDialogManager::RunJavaScriptDialog(
    content::WebContents* web_contents,
    content::RenderFrameHost* render_frame_host,
    content::JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    DialogClosedCallback callback,
    bool* did_suppress_message) {
  BisonContentsClientBridge* bridge =
      BisonContentsClientBridge::FromWebContents(web_contents);
  if (!bridge) {
    std::move(callback).Run(false, base::string16());
    return;
  }

  bridge->RunJavaScriptDialog(
      dialog_type, render_frame_host->GetLastCommittedURL(), message_text,
      default_prompt_text, std::move(callback));
}

void BisonJavaScriptDialogManager::RunBeforeUnloadDialog(
    content::WebContents* web_contents,
    content::RenderFrameHost* render_frame_host,
    bool is_reload,
    DialogClosedCallback callback) {
  BisonContentsClientBridge* bridge =
      BisonContentsClientBridge::FromWebContents(web_contents);
  if (!bridge) {
    std::move(callback).Run(false, base::string16());
    return;
  }

  bridge->RunBeforeUnloadDialog(web_contents->GetURL(), std::move(callback));
}

void BisonJavaScriptDialogManager::CancelDialogs(
    content::WebContents* web_contents,
    bool reset_state) {}

}  // namespace bison
