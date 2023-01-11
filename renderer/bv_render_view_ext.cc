#include "bison/renderer/bv_render_view_ext.h"

#include "bison/common/mojom/frame.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_view.h"

namespace bison {

BvRenderViewExt::BvRenderViewExt(blink::WebView* web_view)
    : blink::WebViewObserver(web_view) {
  DCHECK(web_view != nullptr);
}

BvRenderViewExt::~BvRenderViewExt() {}

// static
void BvRenderViewExt::WebViewCreated(blink::WebView* web_view) {
  new BvRenderViewExt(web_view);  // |render_view| takes ownership.
}

void BvRenderViewExt::DidCommitCompositorFrame() {
  UpdateContentsSize();
}

void BvRenderViewExt::DidUpdateMainFrameLayout() {
  // The size may have changed.
  needs_contents_size_update_ = true;
}

void BvRenderViewExt::OnDestruct() {
  delete this;
}

void BvRenderViewExt::UpdateContentsSize() {
  blink::WebView* webview = GetWebView();
  blink::WebFrame* main_frame = webview->MainFrame();

  // Even without out-of-process iframes, we now create RemoteFrames for the
  // main frame when you navigate cross-process, to create a placeholder in the
  // old process. This is necessary to support things like postMessage across
  // windows that have references to each other. The RemoteFrame will
  // immediately go away if there aren't any active frames left in the old
  // process. RenderView's main frame pointer will become null in the old
  // process when it is no longer the active main frame.
  if (!main_frame->IsWebLocalFrame())
    return;

  if (!needs_contents_size_update_)
    return;
  needs_contents_size_update_ = false;

  gfx::Size contents_size = main_frame->ToWebLocalFrame()->DocumentSize();

  // Fall back to contentsPreferredMinimumSize if the mainFrame is reporting a
  // 0x0 size (this happens during initial load).
  if (contents_size.IsEmpty()) {
    contents_size = webview->ContentsPreferredMinimumSize();
  }

  if (contents_size == last_sent_contents_size_)
    return;

  last_sent_contents_size_ = contents_size;

  mojo::AssociatedRemote<mojom::FrameHost> frame_host_remote;
  content::RenderFrame::FromWebFrame(main_frame->ToWebLocalFrame())
      ->GetRemoteAssociatedInterfaces()
      ->GetInterface(&frame_host_remote);
  frame_host_remote->ContentsSizeChanged(contents_size);
}

}  // namespace bison
