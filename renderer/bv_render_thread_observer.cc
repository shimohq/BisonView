#include "bison/renderer/bv_render_thread_observer.h"

#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/platform/web_cache.h"
#include "third_party/blink/public/platform/web_network_state_notifier.h"

namespace bison {

BvRenderThreadObserver::BvRenderThreadObserver() {}

BvRenderThreadObserver::~BvRenderThreadObserver() {}

void BvRenderThreadObserver::RegisterMojoInterfaces(
    blink::AssociatedInterfaceRegistry* associated_interfaces) {
  // base::Unretained can be used here because the associated_interfaces
  // is owned by the RenderThread and will live for the duration of the
  // RenderThread.
  associated_interfaces->AddInterface(
      base::BindRepeating(&BvRenderThreadObserver::OnRendererAssociatedRequest,
                          base::Unretained(this)));
}

void BvRenderThreadObserver::UnregisterMojoInterfaces(
    blink::AssociatedInterfaceRegistry* associated_interfaces) {
  associated_interfaces->RemoveInterface(mojom::Renderer::Name_);
}

void BvRenderThreadObserver::OnRendererAssociatedRequest(
    mojo::PendingAssociatedReceiver<mojom::Renderer> receiver) {
  receiver_.Bind(std::move(receiver));
}

void BvRenderThreadObserver::ClearCache() {
  blink::WebCache::Clear();
}

void BvRenderThreadObserver::SetJsOnlineProperty(bool network_up) {
  blink::WebNetworkStateNotifier::SetOnLine(network_up);
}

}  // namespace bison
