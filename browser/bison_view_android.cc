#include <jni.h>

#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "bison/bison_jni_headers/BisonView_jni.h"
#include "bison_view.h"
#include "bison_view_manager.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"

using base::android::AttachCurrentThread;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace bison {

void BisonView::PlatformInitialize(const gfx::Size& default_window_size) {}

void BisonView::PlatformExit() {
  DestroyShellManager();
}

void BisonView::PlatformCleanUp() {
  JNIEnv* env = AttachCurrentThread();
  if (java_object_.is_null())
    return;
  Java_BisonView_onNativeDestroyed(env, java_object_);
}

void BisonView::PlatformEnableUIControl(UIControl control, bool is_enabled) {
  JNIEnv* env = AttachCurrentThread();
  if (java_object_.is_null())
    return;
  Java_BisonView_enableUiControl(env, java_object_, control, is_enabled);
}

void BisonView::PlatformSetAddressBarURL(const GURL& url) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> j_url = ConvertUTF8ToJavaString(env, url.spec());
  Java_BisonView_onUpdateUrl(env, java_object_, j_url);
}

void BisonView::PlatformSetIsLoading(bool loading) {
  JNIEnv* env = AttachCurrentThread();
  Java_BisonView_setIsLoading(env, java_object_, loading);
}

void BisonView::PlatformCreateWindow() {
  java_object_.Reset(CreateShellView(this));
}

void BisonView::PlatformSetContents() {
  JNIEnv* env = AttachCurrentThread();
  Java_BisonView_initFromNativeTabContents(
      env, java_object_, web_contents()->GetJavaWebContents());
}

void BisonView::PlatformResizeSubViews() {
  // Not needed; subviews are bound.
}

void BisonView::SizeTo(const gfx::Size& content_size) {
  JNIEnv* env = AttachCurrentThread();
  Java_BisonView_sizeTo(env, java_object_, content_size.width(),
                        content_size.height());
}

void BisonView::PlatformSetTitle(const base::string16& title) {
  NOTIMPLEMENTED() << ": " << title;
}

void BisonView::LoadProgressChanged(WebContents* source, double progress) {
  JNIEnv* env = AttachCurrentThread();
  Java_BisonView_onLoadProgressChanged(env, java_object_, progress);
}

void BisonView::SetOverlayMode(bool use_overlay_mode) {
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_BisonView_setOverlayMode(env, java_object_, use_overlay_mode);
}

void BisonView::PlatformToggleFullscreenModeForTab(WebContents* web_contents,
                                                   bool enter_fullscreen) {
  JNIEnv* env = AttachCurrentThread();
  Java_BisonView_toggleFullscreenModeForTab(env, java_object_,
                                            enter_fullscreen);
}

bool BisonView::PlatformIsFullscreenForTabOrPending(
    const WebContents* web_contents) const {
  JNIEnv* env = AttachCurrentThread();
  return Java_BisonView_isFullscreenForTabOrPending(env, java_object_);
}

void BisonView::Close() {
  RemoveShellView(java_object_);
  delete this;
}

// static
void JNI_BisonView_CloseShell(JNIEnv* env, jlong bisonViewPtr) {
  BisonView* bisonView = reinterpret_cast<BisonView*>(bisonViewPtr);
  bisonView->Close();
}

}  // namespace bison
