// create by jiang947

#ifndef BISON_RENDERER_BV_RENDER_FRAME_EXT_H_
#define BISON_RENDERER_BV_RENDER_FRAME_EXT_H_

#include "bison/common/mojom/frame.mojom.h"

#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/size_f.h"
#include "url/origin.h"

namespace blink {
class WebFrameWidget;
class WebView;
}  // namespace blink

namespace bison {

class BvRenderFrameExt : public content::RenderFrameObserver ,
                         mojom::LocalMainFrame {
 public:
  explicit BvRenderFrameExt(content::RenderFrame* render_frame);

  BvRenderFrameExt(const BvRenderFrameExt&) = delete;
  BvRenderFrameExt& operator=(const BvRenderFrameExt&) = delete;

  static BvRenderFrameExt* FromRenderFrame(content::RenderFrame* render_frame);

 private:
  ~BvRenderFrameExt() override;

  // RenderFrameObserver:
  bool OnAssociatedInterfaceRequestForFrame(
      const std::string& interface_name,
      mojo::ScopedInterfaceEndpointHandle* handle) override;
  void DidCommitProvisionalLoad(ui::PageTransition transition) override;

  void FocusedElementChanged(const blink::WebElement& element) override;
  void OnDestruct() override;

  // mojom::LocalMainFrame overrides:
  void SetInitialPageScale(double page_scale_factor) override;
  void SetTextZoomFactor(float zoom_factor) override;
  void HitTest(const gfx::PointF& touch_center,
               const gfx::SizeF& touch_area) override;
  void DocumentHasImage(DocumentHasImageCallback callback) override;
  void ResetScrollAndScaleState() override;
  void SmoothScroll(int32_t target_x,
                    int32_t target_y,
                    base::TimeDelta duration) override;

  void BindLocalMainFrame(
      mojo::PendingAssociatedReceiver<mojom::LocalMainFrame> pending_receiver);

  const mojo::AssociatedRemote<mojom::FrameHost>& GetFrameHost();

  blink::WebView* GetWebView();
  blink::WebFrameWidget* GetWebFrameWidget();

  url::Origin last_origin_;

  blink::AssociatedInterfaceRegistry registry_;
  mojo::AssociatedReceiver<mojom::LocalMainFrame> local_main_frame_receiver_{
      this};

  mojo::AssociatedRemote<mojom::FrameHost> frame_host_remote_;
};

}  // namespace bison

#endif  // BISON_RENDERER_BV_RENDER_FRAME_EXT_H_
