// create by jiang947

#ifndef BISON_BROWSER_BISON_SETTINGS_H_
#define BISON_BROWSER_BISON_SETTINGS_H_

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "content/public/browser/web_contents_observer.h"

namespace blink {
namespace web_pref {
struct WebPreferences;
}
}  // namespace blink

namespace bison {

class BvRenderViewHostExt;

class BvSettings : public content::WebContentsObserver {
 public:
  // TODO jiang package name use by BvSettings
  // GENERATED_JAVA_ENUM_PACKAGE: im.shimo.bison.internal
  enum ForceDarkMode {
    FORCE_DARK_OFF = 0,
    FORCE_DARK_AUTO = 1,
    FORCE_DARK_ON = 2,
  };

  // GENERATED_JAVA_ENUM_PACKAGE: im.shimo.bison.internal
  enum ForceDarkBehavior {
    FORCE_DARK_ONLY = 0,
    MEDIA_QUERY_ONLY = 1,
    PREFER_MEDIA_QUERY_OVER_FORCE_DARK = 2,
  };

  // GENERATED_JAVA_ENUM_PACKAGE: im.shimo.bison.internal
  enum RequestedWithHeaderMode {
    NO_HEADER = 0,
    APP_PACKAGE_NAME = 1,
    CONSTANT_WEBVIEW = 2,
  };

  static BvSettings* FromWebContents(content::WebContents* web_contents);
  static bool GetAllowSniffingFileUrls();

static RequestedWithHeaderMode GetDefaultRequestedWithHeaderMode();

  BvSettings(JNIEnv* env, jobject obj, content::WebContents* web_contents);
  ~BvSettings() override;

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
  void UpdateFormDataPreferencesLocked(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
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

  void PopulateWebPreferences(blink::web_pref::WebPreferences* web_prefs);
  bool GetAllowFileAccess();
  bool IsForceDarkApplied(JNIEnv* env,
                          const base::android::JavaParamRef<jobject>& obj);

  void SetEnterpriseAuthenticationAppLinkPolicyEnabled(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      jboolean enabled);
  bool GetEnterpriseAuthenticationAppLinkPolicyEnabled(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  inline bool enterprise_authentication_app_link_policy_enabled() {
    return enterprise_authentication_app_link_policy_enabled_;
  }

 private:
  BvRenderViewHostExt* GetBisonRenderViewHostExt();
  void UpdateEverything();

  // WebContentsObserver overrides:
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;
  void WebContentsDestroyed() override;

  bool renderer_prefs_initialized_;
  bool javascript_can_open_windows_automatically_;
  bool allow_third_party_cookies_;
  bool allow_file_access_;
  bool enterprise_authentication_app_link_policy_enabled_;

  JavaObjectWeakGlobalRef bv_settings_;
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_SETTINGS_H_
