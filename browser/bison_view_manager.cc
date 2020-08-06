#include "bison_view_manager.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/lazy_instance.h"
#include "bison/bison_jni_headers/BisonViewManager_jni.h"
#include "bison_browser_context.h"
#include "bison_content_browser_client.h"
#include "bison_view.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

using base::android::JavaParamRef;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;

namespace {

struct GlobalState {
  GlobalState() {}
  base::android::ScopedJavaGlobalRef<jobject> j_shell_manager;
};

base::LazyInstance<GlobalState>::DestructorAtExit g_global_state =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

namespace bison {

ScopedJavaLocalRef<jobject> CreateShellView(BisonView* shell) {
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_BisonViewManager_createBisonView(
      env, g_global_state.Get().j_shell_manager,
      reinterpret_cast<intptr_t>(shell));
}

void RemoveShellView(const JavaRef<jobject>& shell_view) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BisonViewManager_removeBisonView(
      env, g_global_state.Get().j_shell_manager, shell_view);
}

static void JNI_BisonViewManager_Init(JNIEnv* env,
                                      const JavaParamRef<jobject>& obj) {
  g_global_state.Get().j_shell_manager.Reset(obj);
}

void JNI_BisonViewManager_LaunchShell(JNIEnv* env,
                                      const JavaParamRef<jstring>& jurl) {
  BisonBrowserContext* browserContext =
      BisonContentBrowserClient::Get()->browser_context();
  GURL url(base::android::ConvertJavaStringToUTF8(env, jurl));
  BisonView::CreateNewWindow(browserContext, url, NULL, gfx::Size());
}

void DestroyShellManager() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BisonViewManager_destroy(env, g_global_state.Get().j_shell_manager);
}

}  // namespace bison
