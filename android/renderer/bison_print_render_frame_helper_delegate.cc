// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/renderer/bison_print_render_frame_helper_delegate.h"

#include "third_party/blink/public/web/web_element.h"

namespace bison {

BisonPrintRenderFrameHelperDelegate::BisonPrintRenderFrameHelperDelegate() = default;

BisonPrintRenderFrameHelperDelegate::~BisonPrintRenderFrameHelperDelegate() = default;

bool BisonPrintRenderFrameHelperDelegate::CancelPrerender(
    content::RenderFrame* render_frame) {
  return false;
}

blink::WebElement BisonPrintRenderFrameHelperDelegate::GetPdfElement(
    blink::WebLocalFrame* frame) {
  return blink::WebElement();
}

bool BisonPrintRenderFrameHelperDelegate::IsPrintPreviewEnabled() {
  return false;
}

bool BisonPrintRenderFrameHelperDelegate::IsScriptedPrintEnabled() {
  return false;
}

bool BisonPrintRenderFrameHelperDelegate::OverridePrint(
    blink::WebLocalFrame* frame) {
  return false;
}

}  // namespace bison
