// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_BROWSER_BISON_WEB_CONTENTS_VIEW_DELEGATE_H_
#define BISON_CORE_BROWSER_BISON_WEB_CONTENTS_VIEW_DELEGATE_H_

#include "content/public/browser/web_contents_view_delegate.h"

#include "base/macros.h"

namespace content {
class WebContents;
}  // namespace content

namespace bison {

class BisonWebContentsViewDelegate : public content::WebContentsViewDelegate {
 public:
  static content::WebContentsViewDelegate* Create(
      content::WebContents* web_contents);

  ~BisonWebContentsViewDelegate() override;

  // content::WebContentsViewDelegate implementation.
  content::WebDragDestDelegate* GetDragDestDelegate() override;

 private:
  BisonWebContentsViewDelegate(content::WebContents* web_contents);

  DISALLOW_COPY_AND_ASSIGN(BisonWebContentsViewDelegate);
};

}  // namespace bison

#endif  // BISON_CORE_BROWSER_BISON_WEB_CONTENTS_VIEW_DELEGATE_H_