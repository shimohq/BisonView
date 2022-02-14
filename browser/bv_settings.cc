#include "bison/browser/bv_settings.h"

#include <memory>

#include "bison/bison_jni_headers/BvSettings_jni.h"
#include "bison/browser/bv_browser_context.h"
#include "bison/browser/bv_content_browser_client.h"
#include "bison/browser/bv_contents.h"
#include "bison/browser/renderer_host/bv_render_view_host_ext.h"
#include "bison/common/bv_content_client.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/macros.h"
#include "base/supports_user_data.h"
#include "components/viz/common/features.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/renderer_preferences_util.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/web_preferences.h"
#include "net/http/http_util.h"
#include "third_party/blink/public/mojom/renderer_preferences.mojom.h"

using base::android::ConvertJavaStringToUTF16;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using content::WebPreferences;

namespace bison {

namespace {

void PopulateFixedWebPreferences(WebPreferences* web_prefs) {
  web_prefs->shrinks_standalone_images_to_fit = false;
  web_prefs->should_clear_document_background = false;
  web_prefs->viewport_meta_enabled = true;
  web_prefs->picture_in_picture_enabled = false;
  web_prefs->disable_features_depending_on_viz = true;
  web_prefs->disable_accelerated_small_canvases = true;
  web_prefs->reenable_web_components_v0 = true;
  // WebView has historically not adjusted font scale for text autosizing.
  web_prefs->device_scale_adjustment = 1.0;
}

const void* const kBvSettingsUserDataKey = &kBvSettingsUserDataKey;

}  // namespace

class BvSettingsUserData : public base::SupportsUserData::Data {
 public:
  explicit BvSettingsUserData(BvSettings* ptr) : settings_(ptr) {}

  static BvSettings* GetSettings(content::WebContents* web_contents) {
    if (!web_contents)
      return NULL;
    BvSettingsUserData* data = static_cast<BvSettingsUserData*>(
        web_contents->GetUserData(kBvSettingsUserDataKey));
    return data ? data->settings_ : NULL;
  }

 private:
  BvSettings* settings_;
};

BvSettings::BvSettings(JNIEnv* env,
                             jobject obj,
                             content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      renderer_prefs_initialized_(false),
      javascript_can_open_windows_automatically_(false),
      allow_third_party_cookies_(false),
      allow_file_access_(false),
      bison_settings_(env, obj) {
  web_contents->SetUserData(kBvSettingsUserDataKey,
                            std::make_unique<BvSettingsUserData>(this));
}

BvSettings::~BvSettings() {
  if (web_contents()) {
    web_contents()->SetUserData(kBvSettingsUserDataKey, NULL);
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> scoped_obj = bison_settings_.get(env);
  if (scoped_obj.is_null())
    return;
  Java_BvSettings_nativeBvSettingsGone(env, scoped_obj,
                                             reinterpret_cast<intptr_t>(this));
}

bool BvSettings::GetJavaScriptCanOpenWindowsAutomatically() {
  return javascript_can_open_windows_automatically_;
}

bool BvSettings::GetAllowThirdPartyCookies() {
  return allow_third_party_cookies_;
}

void BvSettings::Destroy(JNIEnv* env, const JavaParamRef<jobject>& obj) {
  delete this;
}

BvSettings* BvSettings::FromWebContents(
    content::WebContents* web_contents) {
  return BvSettingsUserData::GetSettings(web_contents);
}

bool BvSettings::GetAllowSniffingFileUrls() {
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_BvSettings_getAllowSniffingFileUrls(env);
}

BvRenderViewHostExt* BvSettings::GetBisonRenderViewHostExt() {
  if (!web_contents())
    return NULL;
  BvContents* contents = BvContents::FromWebContents(web_contents());
  if (!contents)
    return NULL;
  return contents->render_view_host_ext();
}

void BvSettings::ResetScrollAndScaleState(JNIEnv* env,
                                             const JavaParamRef<jobject>& obj) {
  BvRenderViewHostExt* rvhe = GetBisonRenderViewHostExt();
  if (!rvhe)
    return;
  rvhe->ResetScrollAndScaleState();
}

void BvSettings::UpdateEverything() {
  JNIEnv* env = base::android::AttachCurrentThread();
  CHECK(env);
  ScopedJavaLocalRef<jobject> scoped_obj = bison_settings_.get(env);
  if (scoped_obj.is_null())
    return;
  // Grab the lock and call UpdateEverythingLocked.
  Java_BvSettings_updateEverything(env, scoped_obj);
}

void BvSettings::UpdateEverythingLocked(JNIEnv* env,
                                           const JavaParamRef<jobject>& obj) {
  UpdateInitialPageScaleLocked(env, obj);
  UpdateWebkitPreferencesLocked(env, obj);
  UpdateUserAgentLocked(env, obj);
  ResetScrollAndScaleState(env, obj);
  UpdateFormDataPreferencesLocked(env, obj);
  UpdateRendererPreferencesLocked(env, obj);
  UpdateOffscreenPreRasterLocked(env, obj);
  UpdateWillSuppressErrorStateLocked(env, obj);
  UpdateCookiePolicyLocked(env, obj);
  UpdateAllowFileAccessLocked(env, obj);
}

void BvSettings::UpdateUserAgentLocked(JNIEnv* env,
                                          const JavaParamRef<jobject>& obj) {
  if (!web_contents())
    return;

  ScopedJavaLocalRef<jstring> str =
      Java_BvSettings_getUserAgentLocked(env, obj);
  bool ua_overidden = str.obj() != NULL;

  if (ua_overidden) {
    std::string override = base::android::ConvertJavaStringToUTF8(str);
    web_contents()->SetUserAgentOverride(
        blink::UserAgentOverride::UserAgentOnly(override), true);
  }

  content::NavigationController& controller = web_contents()->GetController();
  for (int i = 0; i < controller.GetEntryCount(); ++i)
    controller.GetEntryAtIndex(i)->SetIsOverridingUserAgent(ua_overidden);
}

void BvSettings::UpdateWebkitPreferencesLocked(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  if (!web_contents())
    return;
  BvRenderViewHostExt* render_view_host_ext = GetBisonRenderViewHostExt();
  if (!render_view_host_ext)
    return;

  content::RenderViewHost* render_view_host =
      web_contents()->GetRenderViewHost();
  if (!render_view_host)
    return;
  render_view_host->OnWebkitPreferencesChanged();
}

void BvSettings::UpdateInitialPageScaleLocked(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  BvRenderViewHostExt* rvhe = GetBisonRenderViewHostExt();
  if (!rvhe)
    return;

  float initial_page_scale_percent =
      Java_BvSettings_getInitialPageScalePercentLocked(env, obj);
  if (initial_page_scale_percent == 0) {
    rvhe->SetInitialPageScale(-1);
  } else {
    float dip_scale =
        static_cast<float>(Java_BvSettings_getDIPScaleLocked(env, obj));
    rvhe->SetInitialPageScale(initial_page_scale_percent / dip_scale / 100.0f);
  }
}

void BvSettings::UpdateWillSuppressErrorStateLocked(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  BvRenderViewHostExt* rvhe = GetBisonRenderViewHostExt();
  if (!rvhe)
    return;

  bool suppress = Java_BvSettings_getWillSuppressErrorPageLocked(env, obj);
  rvhe->SetWillSuppressErrorPage(suppress);
}

void BvSettings::UpdateFormDataPreferencesLocked(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  if (!web_contents())
    return;
  BvContents* contents = BvContents::FromWebContents(web_contents());
  if (!contents)
    return;

  contents->SetSaveFormData(Java_BvSettings_getSaveFormDataLocked(env, obj));
}

void BvSettings::UpdateRendererPreferencesLocked(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  if (!web_contents())
    return;

  bool update_prefs = false;
  blink::mojom::RendererPreferences* prefs =
      web_contents()->GetMutableRendererPrefs();

  if (!renderer_prefs_initialized_) {
    content::UpdateFontRendererPreferencesFromSystemSettings(prefs);
    renderer_prefs_initialized_ = true;
    update_prefs = true;
  }

  if (prefs->accept_languages.compare(
          BvContentBrowserClient::GetAcceptLangsImpl())) {
    prefs->accept_languages = BvContentBrowserClient::GetAcceptLangsImpl();
    update_prefs = true;
  }

  if (update_prefs)
    web_contents()->SyncRendererPrefs();

  if (update_prefs) {
    // make sure to update accept languages when the network service is enabled
    BvBrowserContext* bv_browser_context =
        BvBrowserContext::FromWebContents(web_contents());
    // AndroidWebview does not use per-site storage partitions.
    content::StoragePartition* storage_partition =
        content::BrowserContext::GetDefaultStoragePartition(
            bv_browser_context);
    std::string expanded_language_list =
        net::HttpUtil::ExpandLanguageList(prefs->accept_languages);
    storage_partition->GetNetworkContext()->SetAcceptLanguage(
        net::HttpUtil::GenerateAcceptLanguageHeader(expanded_language_list));
  }
}

void BvSettings::UpdateCookiePolicyLocked(JNIEnv* env,
                                             const JavaParamRef<jobject>& obj) {
  if (!web_contents())
    return;

  allow_third_party_cookies_ =
      Java_BvSettings_getAcceptThirdPartyCookiesLocked(env, obj);
}

void BvSettings::UpdateOffscreenPreRasterLocked(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  // jiang
  BvContents* contents = BvContents::FromWebContents(web_contents());
  if (contents) {
    contents->SetOffscreenPreRaster(
        Java_BvSettings_getOffscreenPreRasterLocked(env, obj));
  }
}

void BvSettings::UpdateAllowFileAccessLocked(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  if (!web_contents())
    return;

  allow_file_access_ = Java_BvSettings_getAllowFileAccess(env, obj);
}

void BvSettings::RenderViewHostChanged(content::RenderViewHost* old_host,
                                          content::RenderViewHost* new_host) {
  DCHECK_EQ(new_host, web_contents()->GetRenderViewHost());

  UpdateEverything();
}

void BvSettings::WebContentsDestroyed() {
  delete this;
}

void BvSettings::PopulateWebPreferences(WebPreferences* web_prefs) {
  JNIEnv* env = base::android::AttachCurrentThread();
  CHECK(env);
  ScopedJavaLocalRef<jobject> scoped_obj = bison_settings_.get(env);
  if (scoped_obj.is_null())
    return;
  // Grab the lock and call PopulateWebPreferencesLocked.
  Java_BvSettings_populateWebPreferences(env, scoped_obj,
                                            reinterpret_cast<jlong>(web_prefs));
}

void BvSettings::PopulateWebPreferencesLocked(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jlong web_prefs_ptr) {
  BvRenderViewHostExt* render_view_host_ext = GetBisonRenderViewHostExt();
  if (!render_view_host_ext)
    return;

  WebPreferences* web_prefs = reinterpret_cast<WebPreferences*>(web_prefs_ptr);
  PopulateFixedWebPreferences(web_prefs);

  web_prefs->text_autosizing_enabled =
      Java_BvSettings_getTextAutosizingEnabledLocked(env, obj);

  int text_size_percent = Java_BvSettings_getTextSizePercentLocked(env, obj);
  if (web_prefs->text_autosizing_enabled) {
    web_prefs->font_scale_factor = text_size_percent / 100.0f;
    web_prefs->force_enable_zoom = text_size_percent >= 130;
    // Use the default zoom factor value when Text Autosizer is turned on.
    render_view_host_ext->SetTextZoomFactor(1);
  } else {
    web_prefs->force_enable_zoom = false;
    render_view_host_ext->SetTextZoomFactor(text_size_percent / 100.0f);
  }

  web_prefs->standard_font_family_map[content::kCommonScript] =
      ConvertJavaStringToUTF16(
          Java_BvSettings_getStandardFontFamilyLocked(env, obj));

  web_prefs->fixed_font_family_map[content::kCommonScript] =
      ConvertJavaStringToUTF16(
          Java_BvSettings_getFixedFontFamilyLocked(env, obj));

  web_prefs->sans_serif_font_family_map[content::kCommonScript] =
      ConvertJavaStringToUTF16(
          Java_BvSettings_getSansSerifFontFamilyLocked(env, obj));

  web_prefs->serif_font_family_map[content::kCommonScript] =
      ConvertJavaStringToUTF16(
          Java_BvSettings_getSerifFontFamilyLocked(env, obj));

  web_prefs->cursive_font_family_map[content::kCommonScript] =
      ConvertJavaStringToUTF16(
          Java_BvSettings_getCursiveFontFamilyLocked(env, obj));

  web_prefs->fantasy_font_family_map[content::kCommonScript] =
      ConvertJavaStringToUTF16(
          Java_BvSettings_getFantasyFontFamilyLocked(env, obj));

  web_prefs->default_encoding = ConvertJavaStringToUTF8(
      Java_BvSettings_getDefaultTextEncodingLocked(env, obj));

  web_prefs->minimum_font_size =
      Java_BvSettings_getMinimumFontSizeLocked(env, obj);

  web_prefs->minimum_logical_font_size =
      Java_BvSettings_getMinimumLogicalFontSizeLocked(env, obj);

  web_prefs->default_font_size =
      Java_BvSettings_getDefaultFontSizeLocked(env, obj);

  web_prefs->default_fixed_font_size =
      Java_BvSettings_getDefaultFixedFontSizeLocked(env, obj);

  // Blink's LoadsImagesAutomatically and ImagesEnabled must be
  // set cris-cross to Android's. See
  // https://code.google.com/p/chromium/issues/detail?id=224317#c26
  web_prefs->loads_images_automatically =
      Java_BvSettings_getImagesEnabledLocked(env, obj);
  web_prefs->images_enabled =
      Java_BvSettings_getLoadsImagesAutomaticallyLocked(env, obj);

  web_prefs->javascript_enabled =
      Java_BvSettings_getJavaScriptEnabledLocked(env, obj);

  web_prefs->allow_universal_access_from_file_urls =
      Java_BvSettings_getAllowUniversalAccessFromFileURLsLocked(env, obj);

  web_prefs->allow_file_access_from_file_urls =
      Java_BvSettings_getAllowFileAccessFromFileURLsLocked(env, obj);

  javascript_can_open_windows_automatically_ =
      Java_BvSettings_getJavaScriptCanOpenWindowsAutomaticallyLocked(env,
                                                                        obj);

  web_prefs->supports_multiple_windows =
      Java_BvSettings_getSupportMultipleWindowsLocked(env, obj);

  web_prefs->plugins_enabled = false;

  web_prefs->application_cache_enabled =
      Java_BvSettings_getAppCacheEnabledLocked(env, obj);

  web_prefs->local_storage_enabled =
      Java_BvSettings_getDomStorageEnabledLocked(env, obj);

  web_prefs->databases_enabled =
      Java_BvSettings_getDatabaseEnabledLocked(env, obj);

  web_prefs->wide_viewport_quirk = true;
  web_prefs->use_wide_viewport =
      Java_BvSettings_getUseWideViewportLocked(env, obj);

  web_prefs->force_zero_layout_height =
      Java_BvSettings_getForceZeroLayoutHeightLocked(env, obj);

  const bool zero_layout_height_disables_viewport_quirk =
      Java_BvSettings_getZeroLayoutHeightDisablesViewportQuirkLocked(env,
                                                                        obj);
  web_prefs->viewport_enabled = !(zero_layout_height_disables_viewport_quirk &&
                                  web_prefs->force_zero_layout_height);

  web_prefs->double_tap_to_zoom_enabled =
      Java_BvSettings_supportsDoubleTapZoomLocked(env, obj);

  web_prefs->initialize_at_minimum_page_scale =
      Java_BvSettings_getLoadWithOverviewModeLocked(env, obj);

  web_prefs->autoplay_policy =
      Java_BvSettings_getMediaPlaybackRequiresUserGestureLocked(env, obj)
          ? content::AutoplayPolicy::kUserGestureRequired
          : content::AutoplayPolicy::kNoUserGestureRequired;

  ScopedJavaLocalRef<jstring> url =
      Java_BvSettings_getDefaultVideoPosterURLLocked(env, obj);
  web_prefs->default_video_poster_url =
      url.obj() ? GURL(ConvertJavaStringToUTF8(url)) : GURL();

  bool support_quirks =
      Java_BvSettings_getSupportLegacyQuirksLocked(env, obj);
  // Please see the corresponding Blink settings for bug references.
  web_prefs->support_deprecated_target_density_dpi = support_quirks;
  web_prefs->use_legacy_background_size_shorthand_behavior = support_quirks;
  web_prefs->viewport_meta_merge_content_quirk = support_quirks;
  web_prefs->viewport_meta_non_user_scalable_quirk = support_quirks;
  web_prefs->viewport_meta_zero_values_quirk = support_quirks;
  web_prefs->clobber_user_agent_initial_scale_quirk = support_quirks;
  web_prefs->ignore_main_frame_overflow_hidden_quirk = support_quirks;
  web_prefs->report_screen_size_in_physical_pixels_quirk = support_quirks;

  web_prefs->reuse_global_for_unowned_main_frame =
      Java_BvSettings_getAllowEmptyDocumentPersistenceLocked(env, obj);

  web_prefs->password_echo_enabled =
      Java_BvSettings_getPasswordEchoEnabledLocked(env, obj);
  web_prefs->spatial_navigation_enabled =
      Java_BvSettings_getSpatialNavigationLocked(env, obj);

  bool enable_supported_hardware_accelerated_features =
      Java_BvSettings_getEnableSupportedHardwareAcceleratedFeaturesLocked(
          env, obj);
  web_prefs->accelerated_2d_canvas_enabled =
      web_prefs->accelerated_2d_canvas_enabled &&
      enable_supported_hardware_accelerated_features;
  // Always allow webgl. Webview always requires access to the GPU even if
  // it only does software draws. WebGL will not show up in software draw so
  // there is no more brokenness for user. This makes it easier for apps that
  // want to start running webgl content before webview is first attached.

  // If strict mixed content checking is enabled then running should not be
  // allowed.
  DCHECK(!Java_BvSettings_getUseStricMixedContentCheckingLocked(env, obj) ||
         !Java_BvSettings_getAllowRunningInsecureContentLocked(env, obj));
  web_prefs->allow_running_insecure_content =
      Java_BvSettings_getAllowRunningInsecureContentLocked(env, obj);
  web_prefs->strict_mixed_content_checking =
      Java_BvSettings_getUseStricMixedContentCheckingLocked(env, obj);

  web_prefs->fullscreen_supported =
      Java_BvSettings_getFullscreenSupportedLocked(env, obj);
  web_prefs->record_whole_document =
      Java_BvSettings_getRecordFullDocument(env, obj);

  // TODO(jww): This should be removed once sufficient warning has been given of
  // possible API breakage because of disabling insecure use of geolocation.
  web_prefs->allow_geolocation_on_insecure_origins =
      Java_BvSettings_getAllowGeolocationOnInsecureOrigins(env, obj);

  web_prefs->do_not_update_selection_on_mutating_selection_range =
      Java_BvSettings_getDoNotUpdateSelectionOnMutatingSelectionRange(env,
                                                                         obj);

  web_prefs->css_hex_alpha_color_enabled =
      Java_BvSettings_getCSSHexAlphaColorEnabledLocked(env, obj);

  // Keep spellcheck disabled on html elements unless the spellcheck="true"
  // attribute is explicitly specified. This "opt-in" behavior is for backward
  // consistency in apps that use WebView (see crbug.com/652314).
  web_prefs->spellcheck_enabled_by_default = false;

  web_prefs->scroll_top_left_interop_enabled =
      Java_BvSettings_getScrollTopLeftInteropEnabledLocked(env, obj);

  // jiang 暂时 注释 dark_mode 判断
  // bool is_dark_mode;
  // switch (Java_BvSettings_getForceDarkModeLocked(env, obj)) {
  //   case ForceDarkMode::FORCE_DARK_OFF:
  //     is_dark_mode = false;
  //     break;
  //   case ForceDarkMode::FORCE_DARK_ON:
  //     is_dark_mode = true;
  //     break;
  //   case ForceDarkMode::FORCE_DARK_AUTO: {
  //     BvContents* contents =
  //     BvContents::FromWebContents(web_contents()); is_dark_mode = contents
  //     && contents->GetViewTreeForceDarkState(); break;
  //   }
  // }
  // web_prefs->preferred_color_scheme =
  //     is_dark_mode ? blink::PreferredColorScheme::kDark
  //                  : blink::PreferredColorScheme::kNoPreference;
  // if (is_dark_mode) {
  //   switch (Java_BvSettings_getForceDarkBehaviorLocked(env, obj)) {
  //     case ForceDarkBehavior::FORCE_DARK_ONLY: {
  //       web_prefs->preferred_color_scheme =
  //           blink::PreferredColorScheme::kNoPreference;
  //       web_prefs->force_dark_mode_enabled = true;
  //       break;
  //     }
  //     case ForceDarkBehavior::MEDIA_QUERY_ONLY: {
  //       web_prefs->preferred_color_scheme =
  //       blink::PreferredColorScheme::kDark;
  //       web_prefs->force_dark_mode_enabled = false;
  //       break;
  //     }
  //     // Blink's behavior is that if the preferred color scheme matches the
  //     // supported color scheme, then force dark will be disabled, otherwise
  //     // the preferred color scheme will be reset to no preference. Therefore
  //     // when enabling force dark, we also set the preferred color scheme to
  //     // dark so that dark themed content will be preferred over force
  //     // darkening.
  //     case ForceDarkBehavior::PREFER_MEDIA_QUERY_OVER_FORCE_DARK: {
  //       web_prefs->preferred_color_scheme =
  //       blink::PreferredColorScheme::kDark;
  //       web_prefs->force_dark_mode_enabled = true;
  //       break;
  //     }
  //   }
  // } else {
  //   web_prefs->preferred_color_scheme =
  //       blink::PreferredColorScheme::kNoPreference;
  //   web_prefs->force_dark_mode_enabled = false;
  // }
}

bool BvSettings::GetAllowFileAccess() {
  return allow_file_access_;
}

static jlong JNI_BvSettings_Init(JNIEnv* env,
                                    const JavaParamRef<jobject>& obj,
                                    const JavaParamRef<jobject>& web_contents) {
  content::WebContents* contents =
      content::WebContents::FromJavaWebContents(web_contents);
  BvSettings* settings = new BvSettings(env, obj, contents);
  return reinterpret_cast<intptr_t>(settings);
}

static ScopedJavaLocalRef<jstring> JNI_BvSettings_GetDefaultUserAgent(
    JNIEnv* env) {
  return base::android::ConvertUTF8ToJavaString(env, GetUserAgent());
}

}  // namespace bison
