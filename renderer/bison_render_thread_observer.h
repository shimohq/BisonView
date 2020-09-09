// create by jiang947

#ifndef BISON_RENDERER_BISON_RENDER_THREAD_OBSERVER_H_
#define BISON_RENDERER_BISON_RENDER_THREAD_OBSERVER_H_

#include "base/compiler_specific.h"
#include "content/public/renderer/render_thread_observer.h"

namespace bison {

// A RenderThreadObserver implementation used for handling android_webview
// specific render-process wide IPC messages.
class BisonRenderThreadObserver : public content::RenderThreadObserver {
 public:
  BisonRenderThreadObserver();
  ~BisonRenderThreadObserver() override;

  // content::RenderThreadObserver implementation.
  bool OnControlMessageReceived(const IPC::Message& message) override;

 private:
  void OnClearCache();
  void OnKillProcess();
  void OnSetJsOnlineProperty(bool network_up);
};

}  // namespace bison

#endif  // BISON_RENDERER_BISON_RENDER_THREAD_OBSERVER_H_
