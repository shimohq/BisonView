// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_BROWSER_BISON_JAVASCRIPT_DIALOG_MANAGER_H_
#define BISON_ANDROID_BROWSER_BISON_JAVASCRIPT_DIALOG_MANAGER_H_

#include "base/macros.h"
#include "content/public/browser/javascript_dialog_manager.h"

namespace bison {

class BisonJavaScriptDialogManager : public content::JavaScriptDialogManager {
 public:
  BisonJavaScriptDialogManager();
  ~BisonJavaScriptDialogManager() override;

  // Overridden from content::JavaScriptDialogManager:
  void RunJavaScriptDialog(content::WebContents* web_contents,
                           content::RenderFrameHost* render_frame_host,
                           content::JavaScriptDialogType dialog_type,
                           const base::string16& message_text,
                           const base::string16& default_prompt_text,
                           DialogClosedCallback callback,
                           bool* did_suppress_message) override;
  void RunBeforeUnloadDialog(content::WebContents* web_contents,
                             content::RenderFrameHost* render_frame_host,
                             bool is_reload,
                             DialogClosedCallback callback) override;
  void CancelDialogs(content::WebContents* web_contents,
                     bool reset_state) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BisonJavaScriptDialogManager);
};

}  // namespace bison

#endif  // BISON_ANDROID_BROWSER_BISON_JAVASCRIPT_DIALOG_MANAGER_H_
