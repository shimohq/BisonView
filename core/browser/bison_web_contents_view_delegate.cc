// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/core/browser/bison_web_contents_view_delegate.h"

#include "content/public/browser/web_contents.h"
#include "content/public/common/context_menu_params.h"
#include "ui/gfx/color_space.h"

namespace bison {

// static
content::WebContentsViewDelegate* BisonWebContentsViewDelegate::Create(
    content::WebContents* web_contents) {
  return new BisonWebContentsViewDelegate(web_contents);
}

BisonWebContentsViewDelegate::BisonWebContentsViewDelegate(
    content::WebContents* web_contents) {
  // Cannot instantiate web_contents_view_delegate_ here because
  // BisonContents::SetWebDelegate is not called yet.
}

BisonWebContentsViewDelegate::~BisonWebContentsViewDelegate() {}

content::WebDragDestDelegate* BisonWebContentsViewDelegate::GetDragDestDelegate() {
  // GetDragDestDelegate is a pure virtual method from WebContentsViewDelegate
  // and must have an implementation although android doesn't use it.
  NOTREACHED();
  return NULL;
}

}  // namespace bison
