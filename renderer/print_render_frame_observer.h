// create by jiang947 


#ifndef BISON_RENDERER_PRINT_RENDER_FRAME_OBSERVER_H_
#define BISON_RENDERER_PRINT_RENDER_FRAME_OBSERVER_H_


#include "base/macros.h"
#include "content/public/renderer/render_frame_observer.h"

namespace bison {

class PrintRenderFrameObserver : public content::RenderFrameObserver {
 public:
  explicit PrintRenderFrameObserver(content::RenderFrame* render_view);

 private:
  ~PrintRenderFrameObserver() override;

  // RenderFrameObserver implementation.
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnDestruct() override;

  // IPC handlers
  void OnPrintNodeUnderContextMenu();

  DISALLOW_COPY_AND_ASSIGN(PrintRenderFrameObserver);
};

}  // namespace bison

#endif  // BISON_RENDERER_PRINT_RENDER_FRAME_OBSERVER_H_
