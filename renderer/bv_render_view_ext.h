// create by jiang947

#ifndef BISON_RENDERER_BV_RENDER_VIEW_EXT_H_
#define BISON_RENDERER_BV_RENDER_VIEW_EXT_H_

#include "base/timer/timer.h"
#include "third_party/blink/public/web/web_view_observer.h"
#include "ui/gfx/geometry/size.h"

namespace bison {

// NOTE: We should not add more things to RenderView and related classes.
//       RenderView is deprecated in content, since it is not compatible
//       with site isolation/out of process iframes.

// Render process side of BvRenderViewHostExt, this provides cross-process
// implementation of miscellaneous WebView functions that we need to poke
// WebKit directly to implement (and that aren't needed in the chrome app).
class BvRenderViewExt : public blink::WebViewObserver {
 public:
  BvRenderViewExt(const BvRenderViewExt&) = delete;
  BvRenderViewExt& operator=(const BvRenderViewExt&) = delete;

  static void WebViewCreated(blink::WebView* web_view);

 private:
  BvRenderViewExt(blink::WebView* web_view);
  ~BvRenderViewExt() override;

  // blink::WebViewObserver overrides.
  void DidCommitCompositorFrame() override;
  void DidUpdateMainFrameLayout() override;
  void OnDestruct() override;

  void UpdateContentsSize();

  gfx::Size last_sent_contents_size_;

  // Whether the contents size may have changed and |UpdateContentsSize| needs
  // to be called.
  bool needs_contents_size_update_ = true;
};

}  // namespace bison

#endif  // BISON_RENDERER_BV_RENDER_VIEW_EXT_H_
