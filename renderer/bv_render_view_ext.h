// create by jiang947

#ifndef BISON_RENDERER_BISON_RENDER_VIEW_EXT_H_
#define BISON_RENDERER_BISON_RENDER_VIEW_EXT_H_

#include "base/timer/timer.h"
#include "content/public/renderer/render_view_observer.h"
#include "ui/gfx/geometry/size.h"

namespace bison {

// NOTE: We should not add more things to RenderView and related classes.
//       RenderView is deprecated in content, since it is not compatible
//       with site isolation/out of process iframes.

// Render process side of BvRenderViewHostExt, this provides cross-process
// implementation of miscellaneous WebView functions that we need to poke
// WebKit directly to implement (and that aren't needed in the chrome app).
class BvRenderViewExt : public content::RenderViewObserver {
 public:
  static void RenderViewCreated(content::RenderView* render_view);

 private:
  BvRenderViewExt(content::RenderView* render_view);
  ~BvRenderViewExt() override;

  // RenderViewObserver:
  void DidCommitCompositorFrame() override;
  void DidUpdateMainFrameLayout() override;
  void OnDestruct() override;

  void UpdateContentsSize();

  gfx::Size last_sent_contents_size_;

  // Whether the contents size may have changed and |UpdateContentsSize| needs
  // to be called.
  bool needs_contents_size_update_ = true;

  DISALLOW_COPY_AND_ASSIGN(BvRenderViewExt);
};

}  // namespace bison

#endif  // BISON_RENDERER_BISON_RENDER_VIEW_EXT_H_
