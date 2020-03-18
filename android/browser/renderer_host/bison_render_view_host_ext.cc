// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/browser/renderer_host/bison_render_view_host_ext.h"

#include "bison/android/browser/bison_browser_context.h"
#include "bison/android/common/render_view_messages.h"
#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace bison {

BisonRenderViewHostExt::BisonRenderViewHostExt(BisonRenderViewHostExtClient* client,
                                         content::WebContents* contents)
    : content::WebContentsObserver(contents),
      client_(client),
      background_color_(SK_ColorWHITE),
      has_new_hit_test_data_(false) {
  DCHECK(client_);
}

BisonRenderViewHostExt::~BisonRenderViewHostExt() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ClearImageRequests();
}

void BisonRenderViewHostExt::DocumentHasImages(DocumentHasImagesResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!web_contents()->GetRenderViewHost()) {
    std::move(result).Run(false);
    return;
  }
  static uint32_t next_id = 1;
  uint32_t this_id = next_id++;
  // Send the message to the main frame, instead of the whole frame tree,
  // because it only makes sense on the main frame.
  if (web_contents()->GetMainFrame()->Send(new AwViewMsg_DocumentHasImages(
          web_contents()->GetMainFrame()->GetRoutingID(), this_id))) {
    image_requests_callback_map_[this_id] = std::move(result);
  } else {
    // Still have to respond to the API call WebView#docuemntHasImages.
    // Otherwise the listener of the response may be starved.
    std::move(result).Run(false);
  }
}

void BisonRenderViewHostExt::ClearCache() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  web_contents()->GetRenderViewHost()->Send(new AwViewMsg_ClearCache);
}

void BisonRenderViewHostExt::KillRenderProcess() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  web_contents()->GetRenderViewHost()->Send(new AwViewMsg_KillProcess);
}

bool BisonRenderViewHostExt::HasNewHitTestData() const {
  return has_new_hit_test_data_;
}

void BisonRenderViewHostExt::MarkHitTestDataRead() {
  has_new_hit_test_data_ = false;
}

void BisonRenderViewHostExt::RequestNewHitTestDataAt(
    const gfx::PointF& touch_center,
    const gfx::SizeF& touch_area) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // We only need to get blink::WebView on the renderer side to invoke the
  // blink hit test API, so sending this IPC to main frame is enough.
  web_contents()->GetMainFrame()->Send(
      new AwViewMsg_DoHitTest(web_contents()->GetMainFrame()->GetRoutingID(),
                              touch_center, touch_area));
}

const BisonHitTestData& BisonRenderViewHostExt::GetLastHitTestData() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return last_hit_test_data_;
}

void BisonRenderViewHostExt::SetTextZoomFactor(float factor) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  web_contents()->GetMainFrame()->Send(new AwViewMsg_SetTextZoomFactor(
      web_contents()->GetMainFrame()->GetRoutingID(), factor));
}

void BisonRenderViewHostExt::ResetScrollAndScaleState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  web_contents()->GetMainFrame()->Send(new AwViewMsg_ResetScrollAndScaleState(
      web_contents()->GetMainFrame()->GetRoutingID()));
}

void BisonRenderViewHostExt::SetInitialPageScale(double page_scale_factor) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  web_contents()->GetMainFrame()->Send(new AwViewMsg_SetInitialPageScale(
      web_contents()->GetMainFrame()->GetRoutingID(), page_scale_factor));
}

void BisonRenderViewHostExt::SetBackgroundColor(SkColor c) {
  if (background_color_ == c)
    return;
  background_color_ = c;
  if (web_contents()->GetRenderViewHost()) {
    web_contents()->GetMainFrame()->Send(new AwViewMsg_SetBackgroundColor(
        web_contents()->GetMainFrame()->GetRoutingID(), background_color_));
  }
}

void BisonRenderViewHostExt::SetWillSuppressErrorPage(bool suppress) {
  // We need to store state on the browser-side, as state might need to be
  // synchronized again later (see BisonRenderViewHostExt::RenderFrameCreated)
  if (will_suppress_error_page_ == suppress)
    return;
  will_suppress_error_page_ = suppress;

  web_contents()->SendToAllFrames(new AwViewMsg_SetBackgroundColor(
      MSG_ROUTING_NONE, will_suppress_error_page_));
}

void BisonRenderViewHostExt::SetJsOnlineProperty(bool network_up) {
  web_contents()->GetRenderViewHost()->Send(
      new AwViewMsg_SetJsOnlineProperty(network_up));
}

void BisonRenderViewHostExt::SmoothScroll(int target_x,
                                       int target_y,
                                       base::TimeDelta duration) {
  web_contents()->GetMainFrame()->Send(
      new AwViewMsg_SmoothScroll(web_contents()->GetMainFrame()->GetRoutingID(),
                                 target_x, target_y, duration));
}

void BisonRenderViewHostExt::RenderViewHostChanged(
    content::RenderViewHost* old_host,
    content::RenderViewHost* new_host) {
  ClearImageRequests();
}

void BisonRenderViewHostExt::ClearImageRequests() {
  for (auto& pair : image_requests_callback_map_) {
    std::move(pair.second).Run(false);
  }

  image_requests_callback_map_.clear();
}

void BisonRenderViewHostExt::RenderFrameCreated(
    content::RenderFrameHost* frame_host) {
  if (!frame_host->GetParent()) {
    frame_host->Send(new AwViewMsg_SetBackgroundColor(
        frame_host->GetRoutingID(), background_color_));
  }

  // Synchronizing error page suppression state down to the renderer cannot be
  // done when RenderViewHostChanged is fired (similar to how other settings do
  // it) because for cross-origin navigations in multi-process mode, the
  // navigation will already have started then. Also, newly created subframes
  // need to inherit the state.
  frame_host->Send(new AwViewMsg_WillSuppressErrorPage(
      frame_host->GetRoutingID(), will_suppress_error_page_));
}

void BisonRenderViewHostExt::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!navigation_handle->HasCommitted() ||
      (!navigation_handle->IsInMainFrame() &&
       !navigation_handle->HasSubframeNavigationEntryCommitted()))
    return;

  BisonBrowserContext::FromWebContents(web_contents())
      ->AddVisitedURLs(navigation_handle->GetRedirectChain());
}

void BisonRenderViewHostExt::OnPageScaleFactorChanged(float page_scale_factor) {
  client_->OnWebLayoutPageScaleFactorChanged(page_scale_factor);
}

bool BisonRenderViewHostExt::OnMessageReceived(
    const IPC::Message& message,
    content::RenderFrameHost* render_frame_host) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(BisonRenderViewHostExt, message,
                                   render_frame_host)
    IPC_MESSAGE_HANDLER(AwViewHostMsg_DocumentHasImagesResponse,
                        OnDocumentHasImagesResponse)
    IPC_MESSAGE_HANDLER(AwViewHostMsg_UpdateHitTestData,
                        OnUpdateHitTestData)
    IPC_MESSAGE_HANDLER(AwViewHostMsg_OnContentsSizeChanged,
                        OnContentsSizeChanged)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void BisonRenderViewHostExt::OnInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe) {
  registry_.TryBindInterface(interface_name, interface_pipe);
}

void BisonRenderViewHostExt::OnDocumentHasImagesResponse(
    content::RenderFrameHost* render_frame_host,
    int msg_id,
    bool has_images) {
  // Only makes sense coming from the main frame of the current frame tree.
  // This matches the current implementation that only cares about if there is
  // an img child node in the main document, and essentially invokes JS:
  // node.getElementsByTagName("img").
  if (render_frame_host != web_contents()->GetMainFrame())
    return;

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::map<int, DocumentHasImagesResult>::iterator pending_req =
      image_requests_callback_map_.find(msg_id);
  if (pending_req == image_requests_callback_map_.end()) {
    DLOG(WARNING) << "unexpected DocumentHasImages Response: " << msg_id;
  } else {
    std::move(pending_req->second).Run(has_images);
    image_requests_callback_map_.erase(pending_req);
  }
}

void BisonRenderViewHostExt::OnUpdateHitTestData(
    content::RenderFrameHost* render_frame_host,
    const BisonHitTestData& hit_test_data) {
  content::RenderFrameHost* main_frame_host = render_frame_host;
  while (main_frame_host->GetParent())
    main_frame_host = main_frame_host->GetParent();

  // Make sense from any frame of the current frame tree, because a focused
  // node could be in either the mainframe or a subframe.
  if (main_frame_host != web_contents()->GetMainFrame())
    return;

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  last_hit_test_data_ = hit_test_data;
  has_new_hit_test_data_ = true;
}

void BisonRenderViewHostExt::OnContentsSizeChanged(
    content::RenderFrameHost* render_frame_host,
    const gfx::Size& contents_size) {
  // Only makes sense coming from the main frame of the current frame tree.
  if (render_frame_host != web_contents()->GetMainFrame())
    return;

  client_->OnWebLayoutContentsSizeChanged(contents_size);
}

}  // namespace bison
