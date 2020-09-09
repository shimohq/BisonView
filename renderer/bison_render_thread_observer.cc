#include "bison/renderer/bison_render_thread_observer.h"

#include "bison/common/render_view_messages.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/blink/public/platform/web_cache.h"
#include "third_party/blink/public/platform/web_network_state_notifier.h"

namespace bison {

BisonRenderThreadObserver::BisonRenderThreadObserver() {}

BisonRenderThreadObserver::~BisonRenderThreadObserver() {}

bool BisonRenderThreadObserver::OnControlMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(BisonRenderThreadObserver, message)
    IPC_MESSAGE_HANDLER(BisonViewMsg_ClearCache, OnClearCache)
    IPC_MESSAGE_HANDLER(BisonViewMsg_KillProcess, OnKillProcess)
    IPC_MESSAGE_HANDLER(BisonViewMsg_SetJsOnlineProperty, OnSetJsOnlineProperty)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void BisonRenderThreadObserver::OnClearCache() {
  blink::WebCache::Clear();
}

void BisonRenderThreadObserver::OnKillProcess() {
  LOG(ERROR) << "Killing process (" << getpid() << ") upon request.";
  kill(getpid(), SIGKILL);
}

void BisonRenderThreadObserver::OnSetJsOnlineProperty(bool network_up) {
  blink::WebNetworkStateNotifier::SetOnLine(network_up);
}

}  // namespace bison
