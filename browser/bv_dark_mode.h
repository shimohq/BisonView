#ifndef BISON_BROWSER_BV_DARK_MODE_H_
#define BISON_BROWSER_BV_DARK_MODE_H_

#include "bison/browser/bv_settings.h"

#include "base/android/jni_weak_ref.h"
#include "base/supports_user_data.h"
#include "content/public/browser/web_contents_observer.h"

namespace bison {
class BvDarkMode : public content::WebContentsObserver,
                   public base::SupportsUserData::Data {
 public:
  BvDarkMode(JNIEnv* env, jobject obj, content::WebContents* web_contents);
  ~BvDarkMode() override;

  static BvDarkMode* FromWebContents(content::WebContents* contents);

  void PopulateWebPreferences(blink::web_pref::WebPreferences* web_prefs,
                              int force_dark_mode,
                              int force_dark_behavior,
                              bool algorithmic_darkening_allowed);

  void DetachFromJavaObject(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller);

  bool is_force_dark_applied() const { return is_force_dark_applied_; }

 private:
  // content::WebContentsObserver
  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override;
  void InferredColorSchemeUpdated(
      absl::optional<blink::mojom::PreferredColorScheme> color_scheme) override;

  void PopulateWebPreferencesForPreT(blink::web_pref::WebPreferences* web_prefs,
                                     int force_dark_mode,
                                     int force_dark_behavior);

  bool IsAppUsingDarkTheme();

  bool is_force_dark_applied_ = false;
  bool prefers_dark_from_theme_ = false;

  JavaObjectWeakGlobalRef jobj_;
};

}  // namespace bison

#endif  // BISON_BROWSER_BV_DARK_MODE_H_
