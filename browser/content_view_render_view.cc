#include "bison/browser/content_view_render_view.h"
#include <android/bitmap.h>
#include <android/native_window_jni.h>

#include <memory>

#include "bison/bison_jni_headers/ContentViewRenderView_jni.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/lazy_instance.h"
#include "cc/layers/layer.h"

#include "content/public/browser/android/compositor.h"
#include "content/public/browser/web_contents.h"
#include "ui/android/view_android.h"
#include "ui/android/window_android.h"
#include "ui/gfx/android/java_bitmap.h"
#include "ui/gfx/geometry/size.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace bison {

ContentViewRenderView::ContentViewRenderView(JNIEnv* env,
                                             jobject obj,
                                             gfx::NativeWindow root_window)
    : root_window_(root_window) {
  java_obj_.Reset(env, obj);
}

ContentViewRenderView::~ContentViewRenderView() {

}

// static
static jlong JNI_ContentViewRenderView_Init(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jobject>& jroot_window_android) {
  gfx::NativeWindow root_window =
      ui::WindowAndroid::FromJavaWindowAndroid(jroot_window_android);
  ContentViewRenderView* content_view_render_view =
      new ContentViewRenderView(env, obj, root_window);
  return reinterpret_cast<intptr_t>(content_view_render_view);
}

void ContentViewRenderView::Destroy(JNIEnv* env) {
  delete this;
}

void ContentViewRenderView::SetCurrentWebContents(
    JNIEnv* env,
    const JavaParamRef<jobject>& jweb_contents) {
  InitCompositor();
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);
  compositor_->SetRootLayer(web_contents
                                ? web_contents->GetNativeView()->GetLayer()
                                : scoped_refptr<cc::Layer>());
}

void ContentViewRenderView::OnPhysicalBackingSizeChanged(
    JNIEnv* env,
    const JavaParamRef<jobject>& jweb_contents,
    jint width,
    jint height) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);
  gfx::Size size(width, height);
  web_contents->GetNativeView()->OnPhysicalBackingSizeChanged(size);
}

void ContentViewRenderView::SurfaceCreated(JNIEnv* env) {
  InitCompositor();
  current_surface_format_ = 0;
}

void ContentViewRenderView::SurfaceDestroyed(JNIEnv* env,
                                             jboolean cache_back_buffer) {
  if (cache_back_buffer)
    compositor_->CacheBackBufferForCurrentSurface();
  compositor_->SetSurface(nullptr, false);
  current_surface_format_ = 0;
}

void ContentViewRenderView::SurfaceChanged(
    JNIEnv* env,
    jboolean can_be_used_with_surface_control,
    jint format,
    jint width,
    jint height,
    const JavaParamRef<jobject>& surface) {
  if (current_surface_format_ != format) {
    current_surface_format_ = format;
    compositor_->SetSurface(surface, can_be_used_with_surface_control);
  }
  compositor_->SetWindowBounds(gfx::Size(width, height));
}

void ContentViewRenderView::SetNeedsRedraw(JNIEnv* env) {
  compositor_->SetNeedsRedraw();
}

void ContentViewRenderView::UpdateLayerTreeHost() {
  // TODO(wkorman): Rename Layout to UpdateLayerTreeHost in all Android
  // Compositor related classes.
}

void ContentViewRenderView::DidSwapFrame(int pending_frames) {
  JNIEnv* env = base::android::AttachCurrentThread();
  if (Java_ContentViewRenderView_didSwapFrame(env, java_obj_)) {
    compositor_->SetNeedsRedraw();
  }
}

void ContentViewRenderView::DidSwapBuffers(const gfx::Size& swap_size) {
  JNIEnv* env = base::android::AttachCurrentThread();
  bool matches_window_bounds = swap_size == compositor_->GetWindowBounds();
  Java_ContentViewRenderView_didSwapBuffers(env, java_obj_,
                                            matches_window_bounds);
}

void ContentViewRenderView::EvictCachedSurface(JNIEnv* env) {
  compositor_->EvictCachedBackBuffer();
}

void ContentViewRenderView::InitCompositor() {
  if (!compositor_)
    compositor_.reset(content::Compositor::Create(this, root_window_));
}

}  // namespace bison
