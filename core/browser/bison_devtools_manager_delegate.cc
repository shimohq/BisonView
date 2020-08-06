// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/core/browser/bison_devtools_manager_delegate.h"

#include "bison/core/browser/gfx/browser_view_renderer.h"
#include "bison/core/common/bison_content_client.h"
#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/web_contents.h"

using content::DevToolsAgentHost;

namespace bison {

BisonDevToolsManagerDelegate::BisonDevToolsManagerDelegate() {
}

BisonDevToolsManagerDelegate::~BisonDevToolsManagerDelegate() {
}

std::string BisonDevToolsManagerDelegate::GetTargetDescription(
    content::WebContents* web_contents) {
  bison::BrowserViewRenderer* bvr =
      bison::BrowserViewRenderer::FromWebContents(web_contents);
  if (!bvr)
    return "";
  base::DictionaryValue description;
  description.SetBoolean("attached", bvr->attached_to_window());
  description.SetBoolean("visible", bvr->IsVisible());
  gfx::Rect screen_rect = bvr->GetScreenRect();
  description.SetInteger("screenX", screen_rect.x());
  description.SetInteger("screenY", screen_rect.y());
  description.SetBoolean("empty", screen_rect.size().IsEmpty());
  if (!screen_rect.size().IsEmpty()) {
    description.SetInteger("width", screen_rect.width());
    description.SetInteger("height", screen_rect.height());
  }
  std::string json;
  base::JSONWriter::Write(description, &json);
  return json;
}

std::string BisonDevToolsManagerDelegate::GetDiscoveryPageHTML() {
  const char html[] =
      "<html>"
      "<head><title>WebView remote debugging</title></head>"
      "<body>Please use <a href=\'chrome://inspect\'>chrome://inspect</a>"
      "</body>"
      "</html>";
  return html;
}

bool BisonDevToolsManagerDelegate::IsBrowserTargetDiscoverable() {
  return true;
}

}  // namespace bison