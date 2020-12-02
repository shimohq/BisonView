// create by jiang947

#ifndef BISON_RENDERER_BISON_RENDER_FRAME_EXT_H_
#define BISON_RENDERER_BISON_RENDER_FRAME_EXT_H_

#include "base/macros.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/size_f.h"
#include "url/origin.h"

namespace blink {
enum WebMeaningfulLayout;
class WebFrameWidget;
class WebView;
}  // namespace blink

namespace bison {

// Render process side of BisonRenderViewHostExt, this provides cross-process
// implementation of miscellaneous WebView functions that we need to poke
// WebKit directly to implement (and that aren't needed in the chrome app).
class BisonRenderFrameExt : public content::RenderFrameObserver {
 public:
  explicit BisonRenderFrameExt(content::RenderFrame* render_frame);

  static BisonRenderFrameExt* FromRenderFrame(content::RenderFrame* render_frame);

  bool GetWillSuppressErrorPage();

 private:
  ~BisonRenderFrameExt() override;

  // RenderFrameObserver:
  bool OnAssociatedInterfaceRequestForFrame(
      const std::string& interface_name,
      mojo::ScopedInterfaceEndpointHandle* handle) override;
  void DidCommitProvisionalLoad(ui::PageTransition transition) override;

  bool OnMessageReceived(const IPC::Message& message) override;
  void FocusedElementChanged(const blink::WebElement& element) override;
  void OnDestruct() override;

  void OnDocumentHasImagesRequest(uint32_t id);
  void OnDoHitTest(const gfx::PointF& touch_center,
                   const gfx::SizeF& touch_area);

  void OnSetTextZoomFactor(float zoom_factor);

  void OnResetScrollAndScaleState();

  void OnSetInitialPageScale(double page_scale_factor);

  void OnSetBackgroundColor(SkColor c);

  void OnSmoothScroll(int target_x, int target_y, base::TimeDelta duration);

  void OnSetWillSuppressErrorPage(bool suppress);

  blink::WebView* GetWebView();
  blink::WebFrameWidget* GetWebFrameWidget();

  url::Origin last_origin_;

  blink::AssociatedInterfaceRegistry registry_;

  // Some WebView users might want to show their own error pages / logic
  bool will_suppress_error_page_ = false;

  DISALLOW_COPY_AND_ASSIGN(BisonRenderFrameExt);
};

}  // namespace bison

#endif  // BISON_RENDERER_BISON_RENDER_FRAME_EXT_H_
