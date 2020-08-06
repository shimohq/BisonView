// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison_render_view_observer.h"

#include "base/command_line.h"
#include "content/public/renderer/render_view.h"
#include "content/public/renderer/render_view_observer.h"
#include "third_party/blink/public/web/web_view.h"

namespace bison {

BisonRenderViewObserver::BisonRenderViewObserver(
    content::RenderView* render_view)
    : content::RenderViewObserver(render_view) {}

BisonRenderViewObserver::~BisonRenderViewObserver() {}

void BisonRenderViewObserver::DidClearWindowObject(
    blink::WebLocalFrame* frame) {}

void BisonRenderViewObserver::OnDestruct() {
  delete this;
}

}  // namespace bison
