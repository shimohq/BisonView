// create by jiang947

#ifndef BISON_BROWSER_CONTENT_VIEW_RENDER_VIEW_H_
#define BISON_BROWSER_CONTENT_VIEW_RENDER_VIEW_H_

#include <memory>

#include "base/android/jni_weak_ref.h"
#include "base/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/android/compositor_client.h"
#include "ui/gfx/native_widget_types.h"

namespace cc {
class Layer;
}

namespace content {
class Compositor;
class WebContents;
}  // namespace content

namespace bison {

class ContentViewRenderView : public content::CompositorClient {
 public:
  ContentViewRenderView(JNIEnv* env,
                        jobject obj,
                        gfx::NativeWindow root_window);

  ContentViewRenderView(const ContentViewRenderView&) = delete;
  ContentViewRenderView& operator=(const ContentViewRenderView&) = delete;

  content::Compositor* compositor() { return compositor_.get(); }

  scoped_refptr<cc::Layer> root_container_layer() {
    return root_container_layer_;
  }

  // Height, in pixels.
  int height() const { return height_; }
  void SetHeightChangedListener(base::RepeatingClosure callback);

  // Methods called from Java via JNI -----------------------------------------
  void Destroy(JNIEnv* env);
  void SetCurrentWebContents(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jweb_contents);
  void OnPhysicalBackingSizeChanged(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jweb_contents,
      jint width,
      jint height,
      jboolean for_config_change);
  void SurfaceCreated(JNIEnv* env);
  void SurfaceDestroyed(JNIEnv* env, jboolean cache_back_buffer);
  void SurfaceChanged(JNIEnv* env,
                      jboolean can_be_used_with_surface_control,
                      jint width,
                      jint height,
                      jboolean transparent_background,
                      const base::android::JavaParamRef<jobject>& new_surface);
  void SetNeedsRedraw(JNIEnv* env);
  void EvictCachedSurface(JNIEnv* env);
  base::android::ScopedJavaLocalRef<jobject> GetResourceManager(JNIEnv* env);
  void UpdateBackgroundColor(JNIEnv* env);
  void SetRequiresAlphaChannel(JNIEnv* env, jboolean requires_alpha_channel);
  void SetDidSwapBuffersCallbackEnabled(JNIEnv* env, jboolean enable);

  // CompositorClient implementation
  void UpdateLayerTreeHost() override;
  void DidSwapFrame(int pending_frames) override;
  void DidSwapBuffers(const gfx::Size& swap_size) override;

 private:
  ~ContentViewRenderView() override;

  void InitCompositor();
  void UpdateWebContentsBaseBackgroundColor();

  base::android::ScopedJavaGlobalRef<jobject> java_obj_;
  bool use_transparent_background_ = false;
  bool requires_alpha_channel_ = false;
  raw_ptr<content::WebContents> web_contents_ = nullptr;

  std::unique_ptr<content::Compositor> compositor_;

  gfx::NativeWindow root_window_;

  // Set as the root-layer of the compositor. Contains |web_contents_layer_|.
  scoped_refptr<cc::Layer> root_container_layer_;
  scoped_refptr<cc::Layer> web_contents_layer_;

  base::RepeatingClosure height_changed_listener_;
  int height_ = 0;
};

}  // namespace bison

#endif  // BISON_BROWSER_CONTENT_VIEW_RENDER_VIEW_H_
