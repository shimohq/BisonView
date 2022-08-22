

#include "bison/browser/renderer_host/bv_render_view_host_ext.h"

#include "bison/browser/bv_browser_context.h"
#include "bison/browser/bv_contents.h"
#include "bison/browser/bv_contents_client_bridge.h"

#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

namespace bison {

namespace {

void ShouldOverrideUrlLoadingOnUI(
    content::WebContents* web_contents,
    const std::u16string& url,
    bool has_user_gesture,
    bool is_redirect,
    bool is_main_frame,
    mojom::FrameHost::ShouldOverrideUrlLoadingCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  bool ignore_navigation = false;
  BvContentsClientBridge* client =
      BvContentsClientBridge::FromWebContents(web_contents);
  if (client) {
    if (!client->ShouldOverrideUrlLoading(url, has_user_gesture, is_redirect,
                                          is_main_frame, &ignore_navigation)) {
      // If the shouldOverrideUrlLoading call caused a java exception we should
      // always return immediately here!
      return;
    }
  } else {
    LOG(WARNING) << "Failed to find the associated render view host for url: "
                 << url;
  }

  std::move(callback).Run(ignore_navigation);
}

}  // namespace

// static
void BvRenderViewHostExt::BindFrameHost(
    mojo::PendingAssociatedReceiver<mojom::FrameHost> receiver,
    content::RenderFrameHost* rfh) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents)
    return;
  auto* bv_contents = BvContents::FromWebContents(web_contents);
  if (!bv_contents)
    return;
  auto* bv_rvh_ext = bv_contents->render_view_host_ext();
  if (!bv_rvh_ext)
    return;
  bv_rvh_ext->frame_host_receivers_.Bind(rfh, std::move(receiver));
}

BvRenderViewHostExt::BvRenderViewHostExt(BvRenderViewHostExtClient* client,
    content::WebContents* contents)
    : content::WebContentsObserver(contents),
      client_(client),
      has_new_hit_test_data_(false),
      frame_host_receivers_(contents, this) {
  DCHECK(client_);
}

BvRenderViewHostExt::~BvRenderViewHostExt() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void BvRenderViewHostExt::DocumentHasImages(DocumentHasImagesResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!web_contents()->GetRenderViewHost()) {
    std::move(result).Run(false);
    return;
  }

  if (auto* local_main_frame_remote = GetLocalMainFrameRemote()) {
    local_main_frame_remote->DocumentHasImage(std::move(result));
  } else {
    // Still have to respond to the API call WebView#docuemntHasImages.
    // Otherwise the listener of the response may be starved.
    std::move(result).Run(false);
  }
}

bool BvRenderViewHostExt::HasNewHitTestData() const {
  return has_new_hit_test_data_;
}

void BvRenderViewHostExt::MarkHitTestDataRead() {
  has_new_hit_test_data_ = false;
}

void BvRenderViewHostExt::RequestNewHitTestDataAt(
    const gfx::PointF& touch_center,
    const gfx::SizeF& touch_area) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // We only need to get blink::WebView on the renderer side to invoke the
  // blink hit test Mojo method, so sending this message via LocalMainFrame
  // interface is enough.
  if (auto* local_main_frame_remote = GetLocalMainFrameRemote())
    local_main_frame_remote->HitTest(touch_center, touch_area);
}

const mojom::HitTestData& BvRenderViewHostExt::GetLastHitTestData() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return *last_hit_test_data_;
}

void BvRenderViewHostExt::SetTextZoomFactor(float factor) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (auto* local_main_frame_remote = GetLocalMainFrameRemote())
    local_main_frame_remote->SetTextZoomFactor(factor);
}

void BvRenderViewHostExt::ResetScrollAndScaleState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (auto* local_main_frame_remote = GetLocalMainFrameRemote())
    local_main_frame_remote->ResetScrollAndScaleState();
}

void BvRenderViewHostExt::SetInitialPageScale(double page_scale_factor) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (auto* local_main_frame_remote = GetLocalMainFrameRemote())
    local_main_frame_remote->SetInitialPageScale(page_scale_factor);
}

void BvRenderViewHostExt::SetWillSuppressErrorPage(bool suppress) {
  will_suppress_error_page_ = suppress;
}

void BvRenderViewHostExt::SmoothScroll(int target_x,
                                          int target_y,
                                          base::TimeDelta duration) {
  if (auto* local_main_frame_remote = GetLocalMainFrameRemote())
    local_main_frame_remote->SmoothScroll(target_x, target_y, duration);
  }

void BvRenderViewHostExt::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (will_suppress_error_page_)
    navigation_handle->SetSilentlyIgnoreErrors();
}

void BvRenderViewHostExt::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!navigation_handle->HasCommitted() ||
      (!navigation_handle->IsInMainFrame() &&
       !navigation_handle->HasSubframeNavigationEntryCommitted()))
    return;

  BvBrowserContext::FromWebContents(web_contents())
      ->AddVisitedURLs(navigation_handle->GetRedirectChain());
}

void BvRenderViewHostExt::OnPageScaleFactorChanged(float page_scale_factor) {
  client_->OnWebLayoutPageScaleFactorChanged(page_scale_factor);
}

void BvRenderViewHostExt::UpdateHitTestData(
    mojom::HitTestDataPtr hit_test_data) {
  content::RenderFrameHost* main_frame_host =
      frame_host_receivers_.GetCurrentTargetFrame();
  while (main_frame_host->GetParent())
    main_frame_host = main_frame_host->GetParent();

  // Make sense from any frame of the current frame tree, because a focused
  // node could be in either the mainframe or a subframe.
  if (main_frame_host != web_contents()->GetMainFrame())
    return;

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  last_hit_test_data_ = std::move(hit_test_data);
  has_new_hit_test_data_ = true;
}

void BvRenderViewHostExt::ContentsSizeChanged(const gfx::Size& contents_size) {
  content::RenderFrameHost* render_frame_host =
      frame_host_receivers_.GetCurrentTargetFrame();

  // Only makes sense coming from the main frame of the current frame tree.
  if (render_frame_host != web_contents()->GetMainFrame())
    return;

  client_->OnWebLayoutContentsSizeChanged(contents_size);
}

void BvRenderViewHostExt::ShouldOverrideUrlLoading(
    const std::u16string& url,
    bool has_user_gesture,
    bool is_redirect,
    bool is_main_frame,
    ShouldOverrideUrlLoadingCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(&ShouldOverrideUrlLoadingOnUI, web_contents(),
                                url, has_user_gesture, is_redirect,
                                is_main_frame, std::move(callback)));
}

mojom::LocalMainFrame* BvRenderViewHostExt::GetLocalMainFrameRemote() {
  // Validate the local main frame matches what we have stored for the current
  // main frame. Previously `local_main_frame_remote_` was adjusted in
  // RenderFrameCreated/RenderFrameHostChanged events but the timings of when
  // this class gets called vs others using this class might cause a TOU
  // problem, so we validate it each time before use.
  content::RenderFrameHost* main_frame = web_contents()->GetMainFrame();
  content::GlobalRenderFrameHostId main_frame_id = main_frame->GetGlobalId();
  if (main_frame_global_id_ == main_frame_id) {
    return local_main_frame_remote_.get();
  }

  local_main_frame_remote_.reset();

  // Avoid accessing GetRemoteAssociatedInterfaces until the renderer is
  // created.
  if (!main_frame->IsRenderFrameCreated()) {
    main_frame_global_id_ = content::GlobalRenderFrameHostId();
    return nullptr;
  }

  main_frame_global_id_ = main_frame_id;
  main_frame->GetRemoteAssociatedInterfaces()->GetInterface(
      local_main_frame_remote_.BindNewEndpointAndPassReceiver());
  return local_main_frame_remote_.get();
}

}  // namespace android_webview
