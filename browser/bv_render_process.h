// create by jiang947


#ifndef BISON_BROWSER_BISON_RENDER_PROCESS_H_
#define BISON_BROWSER_BISON_RENDER_PROCESS_H_

#include "bison/common/mojom/renderer.mojom.h"

#include "base/android/scoped_java_ref.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"
#include "content/public/browser/render_process_host_observer.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace bison {

class BvRenderProcess : public content::RenderProcessHostObserver,
                        public base::SupportsUserData::Data {
 public:
  static BvRenderProcess* GetInstanceForRenderProcessHost(
      content::RenderProcessHost* host);

  base::android::ScopedJavaLocalRef<jobject> GetJavaObject();

  bool TerminateChildProcess(JNIEnv* env,
                             const base::android::JavaParamRef<jobject>& obj);

  bool IsProcessLockedToSiteForTesting(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);

  explicit BvRenderProcess(content::RenderProcessHost* render_process_host);

  BvRenderProcess(const BvRenderProcess&) = delete;
  BvRenderProcess& operator=(const BvRenderProcess&) = delete;

  ~BvRenderProcess() override;

  void ClearCache();
  void SetJsOnlineProperty(bool network_up);

 private:
  void Ready();
  void Cleanup();

  // content::RenderProcessHostObserver implementation
  void RenderProcessReady(content::RenderProcessHost* host) override;

  void RenderProcessExited(
      content::RenderProcessHost* host,
      const content::ChildProcessTerminationInfo& info) override;

  base::android::ScopedJavaGlobalRef<jobject> java_obj_;

  raw_ptr<content::RenderProcessHost> render_process_host_;

  mojo::AssociatedRemote<mojom::Renderer> renderer_remote_;

  base::WeakPtrFactory<BvRenderProcess> weak_factory_{this};
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_RENDER_PROCESS_H_
