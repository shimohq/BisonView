// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/renderer/bison_content_settings_client.h"

#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/gurl.h"

namespace bison {

namespace {

bool AllowMixedContent(const blink::WebURL& url) {
  // We treat non-standard schemes as "secure" in the WebView to allow them to
  // be used for request interception.
  // TODO(benm): Tighten this restriction by requiring embedders to register
  // their custom schemes? See b/9420953.
  GURL gurl(url);
  return !gurl.IsStandard();
}

}  // namespace

BisonContentSettingsClient::BisonContentSettingsClient(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
  render_frame->GetWebFrame()->SetContentSettingsClient(this);
}

BisonContentSettingsClient::~BisonContentSettingsClient() {
}

bool BisonContentSettingsClient::AllowRunningInsecureContent(
    bool enabled_per_settings,
    const blink::WebSecurityOrigin& origin,
    const blink::WebURL& url) {
  return enabled_per_settings ? true : AllowMixedContent(url);
}

void BisonContentSettingsClient::OnDestruct() {
  delete this;
}

}  // namespace bison
