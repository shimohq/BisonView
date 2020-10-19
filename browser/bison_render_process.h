// create by jiang947 


#ifndef BISON_BROWSER_BISON_RENDER_PROCESS_H_
#define BISON_BROWSER_BISON_RENDER_PROCESS_H_

#include "base/android/scoped_java_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"

#include "content/public/browser/render_process_host_observer.h"

namespace bison {

class BisonRenderProcess : public content::RenderProcessHostObserver,
                        public base::SupportsUserData::Data {
 public:
  static BisonRenderProcess* GetInstanceForRenderProcessHost(
      content::RenderProcessHost* host);

  base::android::ScopedJavaLocalRef<jobject> GetJavaObject();

  bool TerminateChildProcess(JNIEnv* env,
                             const base::android::JavaParamRef<jobject>& obj);

  explicit BisonRenderProcess(content::RenderProcessHost* render_process_host);
  ~BisonRenderProcess() override;

 private:
  void Ready();
  void Cleanup();

  // content::RenderProcessHostObserver implementation
  void RenderProcessReady(content::RenderProcessHost* host) override;

  void RenderProcessExited(
      content::RenderProcessHost* host,
      const content::ChildProcessTerminationInfo& info) override;

  base::android::ScopedJavaGlobalRef<jobject> java_obj_;

  content::RenderProcessHost* render_process_host_;

  base::WeakPtrFactory<BisonRenderProcess> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(BisonRenderProcess);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_RENDER_PROCESS_H_