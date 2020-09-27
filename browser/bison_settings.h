// create by jiang947

#ifndef BISON_BROWSER_BISON_SETTINGS_H_
#define BISON_BROWSER_BISON_SETTINGS_H_

#include <memory>

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
struct WebPreferences;
}

namespace bison {

class BisonRenderViewHostExt;

class BisonSettings : public content::WebContentsObserver {
 public:
  // TODO jiang package name use by BisonSettings
  // GENERATED_JAVA_ENUM_PACKAGE: im.shimo.bison
  enum ForceDarkMode {
    FORCE_DARK_OFF = 0,
    FORCE_DARK_AUTO = 1,
    FORCE_DARK_ON = 2,
  };

  // GENERATED_JAVA_ENUM_PACKAGE: im.shimo.bison
  enum ForceDarkBehavior {
    FORCE_DARK_ONLY = 0,
    MEDIA_QUERY_ONLY = 1,
    PREFER_MEDIA_QUERY_OVER_FORCE_DARK = 2,
  };

  static BisonSettings* FromWebContents(content::WebContents* web_contents);
  static bool GetAllowSniffingFileUrls();

  BisonSettings(JNIEnv* env, jobject obj, content::WebContents* web_contents);
  ~BisonSettings() override;

  bool GetJavaScriptCanOpenWindowsAutomatically();
  bool GetAllowThirdPartyCookies();

  // Called from Java. Methods with "Locked" suffix require that the settings
  // access lock is held during their execution.
  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void PopulateWebPreferencesLocked(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      jlong web_prefs);
  void ResetScrollAndScaleState(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  void UpdateEverythingLocked(JNIEnv* env,
                              const base::android::JavaParamRef<jobject>& obj);
  void UpdateInitialPageScaleLocked(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  void UpdateWillSuppressErrorStateLocked(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  void UpdateUserAgentLocked(JNIEnv* env,
                             const base::android::JavaParamRef<jobject>& obj);
  void UpdateWebkitPreferencesLocked(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  // void UpdateFormDataPreferencesLocked(
  //     JNIEnv* env,
  //     const base::android::JavaParamRef<jobject>& obj);
  void UpdateRendererPreferencesLocked(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  void UpdateCookiePolicyLocked(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  void UpdateOffscreenPreRasterLocked(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  void UpdateAllowFileAccessLocked(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);

  void PopulateWebPreferences(content::WebPreferences* web_prefs);
  bool GetAllowFileAccess();

 private:
  BisonRenderViewHostExt* GetBisonRenderViewHostExt();
  void UpdateEverything();

  // WebContentsObserver overrides:
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;
  void WebContentsDestroyed() override;

  bool renderer_prefs_initialized_;
  bool javascript_can_open_windows_automatically_;
  bool allow_third_party_cookies_;
  bool allow_file_access_;

  JavaObjectWeakGlobalRef bison_settings_;
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_SETTINGS_H_
