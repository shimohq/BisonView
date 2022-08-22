// create by jiang947

#ifndef BISON_BROWSER_CONTENT_VIEW_RENDER_VIEW_H_
#define BISON_BROWSER_CONTENT_VIEW_RENDER_VIEW_H_

#include <memory>

#include "base/android/jni_weak_ref.h"
#include "base/callback.h"
#include "base/logging.h"

#include "base/memory/weak_ptr.h"
#include "content/public/browser/android/compositor_client.h"
#include "ui/gfx/native_widget_types.h"

namespace cc {
class Layer;
}

namespace content {
class Compositor;
}  // namespace content

namespace bison {

class ContentViewRenderView : public content::CompositorClient {
 public:
  ContentViewRenderView(JNIEnv* env,
                        jobject obj,
                        gfx::NativeWindow root_window);

  // Methods called from Java via JNI -----------------------------------------
  void Destroy(JNIEnv* env);
  void SetCurrentWebContents(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jweb_contents);
  void OnPhysicalBackingSizeChanged(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jweb_contents,
      jint width,
      jint height);
  void SurfaceCreated(JNIEnv* env);
  void SurfaceDestroyed(JNIEnv* env, jboolean cache_back_buffer);
  void SurfaceChanged(JNIEnv* env,
                      jboolean can_be_used_with_surface_control,
                      jint format,
                      jint width,
                      jint height,
                      const base::android::JavaParamRef<jobject>& surface);
  void SetNeedsRedraw(JNIEnv* env);
  void EvictCachedSurface(JNIEnv* env);

  // CompositorClient implementation
  void UpdateLayerTreeHost() override;
  void DidSwapFrame(int pending_frames) override;
  void DidSwapBuffers(const gfx::Size& swap_size) override;

 private:
  ~ContentViewRenderView() override;

  void InitCompositor();

  base::android::ScopedJavaGlobalRef<jobject> java_obj_;

  std::unique_ptr<content::Compositor> compositor_;

  gfx::NativeWindow root_window_;
  int current_surface_format_ = 0;


};

}  // namespace bison

#endif  // BISON_BROWSER_CONTENT_VIEW_RENDER_VIEW_H_
