// create by jiang947

#ifndef BISON_RENDERER_BISON_RENDER_THREAD_OBSERVER_H_
#define BISON_RENDERER_BISON_RENDER_THREAD_OBSERVER_H_

#include "bison/common/mojom/renderer.mojom.h"

#include "base/compiler_specific.h"
#include "content/public/renderer/render_thread_observer.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"

namespace bison {

// A RenderThreadObserver implementation used for handling android_webview
// specific render-process wide IPC messages.
class BvRenderThreadObserver : public content::RenderThreadObserver,
                               public mojom::Renderer {
 public:
  BvRenderThreadObserver();
  ~BvRenderThreadObserver() override;

  // content::RenderThreadObserver implementation.
  void RegisterMojoInterfaces(
      blink::AssociatedInterfaceRegistry* associated_interfaces) override;
  void UnregisterMojoInterfaces(
      blink::AssociatedInterfaceRegistry* associated_interfaces) override;

 private:
  // mojom::Renderer overrides:
  void ClearCache() override;
  void SetJsOnlineProperty(bool network_up) override;

  void OnRendererAssociatedRequest(
      mojo::PendingAssociatedReceiver<mojom::Renderer> receiver);

  mojo::AssociatedReceiver<mojom::Renderer> receiver_{this};
};

}  // namespace bison

#endif  // BISON_RENDERER_BISON_RENDER_THREAD_OBSERVER_H_
