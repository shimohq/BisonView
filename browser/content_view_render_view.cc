#include "bison/browser/content_view_render_view.h"

#include "bison/bison_jni_headers/ContentViewRenderView_jni.h"

#include <android/bitmap.h>
#include <android/native_window_jni.h>

#include <memory>
#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/time/time.h"
#include "cc/layers/layer.h"
#include "cc/layers/picture_layer.h"
#include "content/public/browser/android/compositor.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/android/resources/resource_manager.h"
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
    : root_window_(root_window), current_surface_format_(0) {
  java_obj_.Reset(env, obj);
}

ContentViewRenderView::~ContentViewRenderView() {}

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

void ContentViewRenderView::Destroy(JNIEnv* env,
                                    const JavaParamRef<jobject>& obj) {
  delete this;
}

void ContentViewRenderView::SetCurrentWebContents(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
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
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jobject>& jweb_contents,
    jint width,
    jint height) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);
  gfx::Size size(width, height);
  web_contents->GetNativeView()->OnPhysicalBackingSizeChanged(size);
}

void ContentViewRenderView::SurfaceCreated(JNIEnv* env,
                                           const JavaParamRef<jobject>& obj) {
  current_surface_format_ = 0;
  InitCompositor();
}

void ContentViewRenderView::SurfaceDestroyed(JNIEnv* env,
                                             const JavaParamRef<jobject>& obj) {
  // When we switch from Chrome to other app we can't detach child surface
  // controls because it leads to a visible hole: b/157439199. To avoid this we
  // don't detach surfaces if the surface is going to be destroyed, they will be
  // detached and freed by OS.
  compositor_->PreserveChildSurfaceControls();

  compositor_->SetSurface(nullptr, false);
  current_surface_format_ = 0;
}

void ContentViewRenderView::SurfaceChanged(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint format,
    jint width,
    jint height,
    const JavaParamRef<jobject>& surface) {
  if (current_surface_format_ != format) {
    current_surface_format_ = format;
    compositor_->SetSurface(surface,
                            true /* can_be_used_with_surface_control */);
  }
  compositor_->SetWindowBounds(gfx::Size(width, height));
}

void ContentViewRenderView::SetOverlayVideoMode(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    bool enabled) {
  compositor_->SetRequiresAlphaChannel(enabled);
  compositor_->SetBackgroundColor(enabled ? SK_ColorTRANSPARENT
                                          : SK_ColorWHITE);
  compositor_->SetNeedsComposite();
}

void ContentViewRenderView::UpdateLayerTreeHost() {
  // TODO(wkorman): Rename Layout to UpdateLayerTreeHost in all Android
  // Compositor related classes.
}

void ContentViewRenderView::DidSwapFrame(int pending_frames) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_ContentViewRenderView_didSwapFrame(env, java_obj_);
}

void ContentViewRenderView::InitCompositor() {
  if (!compositor_)
    compositor_.reset(content::Compositor::Create(this, root_window_));
}

}  // namespace bison
