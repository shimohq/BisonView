// create by jiang947

#ifndef BISON_RENDERER_BISON_RENDER_VIEW_OBSERVER_H_
#define BISON_RENDERER_BISON_RENDER_VIEW_OBSERVER_H_

#include "base/macros.h"
#include "content/public/renderer/render_view_observer.h"

namespace blink {
class WebLocalFrame;
}

namespace bison {

class RenderView;

class BisonRenderViewObserver : public content::RenderViewObserver {
 public:
  explicit BisonRenderViewObserver(content::RenderView* render_view);
  ~BisonRenderViewObserver() override;

 private:
  // RenderViewObserver implementation.
  void DidClearWindowObject(blink::WebLocalFrame* frame) override;
  void OnDestruct() override;

  DISALLOW_COPY_AND_ASSIGN(BisonRenderViewObserver);
};

}  // namespace bison

#endif  // BISON_RENDERER_BISON_RENDER_VIEW_OBSERVER_H_
