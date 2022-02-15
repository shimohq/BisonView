// create by jiang947

#ifndef BISON_BROWSER_BISON_VIEW_H_
#define BISON_BROWSER_BISON_VIEW_H_

#include <list>
#include <memory>
#include <string>
#include <utility>

#include "bison/browser/bv_browser_permission_request_delegate.h"
#include "bison/browser/bv_render_process_gone_delegate.h"
#include "bison/browser/permission/permission_request_handler_client.h"
#include "bison/browser/renderer_host/bv_render_view_host_ext.h"
#include "bison/browser/find_helper.h"
#include "bison/browser/icon_helper.h"

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/callback_forward.h"
#include "base/macros.h"
#include "components/js_injection/browser/js_communication_host.h"
#include "content/public/browser/web_contents_observer.h"

class SkBitmap;

namespace autofill {
class AutofillProvider;
}

namespace content {
class WebContents;
}

class GURL;

using base::android::JavaParamRef;
using content::BrowserContext;
using content::NavigationEntry;
using content::OpenURLParams;
using content::RenderFrameHost;
using content::RenderWidgetHost;
using content::SessionStorageNamespace;
using content::SiteInstance;
using content::WebContents;
using content::WebContentsObserver;

namespace bison {

class BvContentsClientBridge;
class BvPdfExporter;
class BvWebContentsDelegate;
class PermissionRequestHandler;

class BvContents : public FindHelper::Listener,
                      public IconHelper::Listener,
                      public BisonRenderViewHostExtClient,
                      public BvRenderProcessGoneDelegate,
                      public PermissionRequestHandlerClient,
                      public BvBrowserPermissionRequestDelegate,
                      public WebContentsObserver {
 public:
  // Returns the BvContents object corresponding to the given WebContents.
  static BvContents* FromWebContents(WebContents* web_contents);
  static BvContents* CreateBisonContents(BrowserContext* browser_context);

  static std::string GetLocale();

  static std::string GetLocaleList();

  BvContents(std::unique_ptr<WebContents> web_contents);
  ~BvContents() override;

  BvRenderViewHostExt* render_view_host_ext() {
    return render_view_host_ext_.get();
  }

  // |handler| is an instance of
  bool OnReceivedHttpAuthRequest(const base::android::JavaRef<jobject>& handler,
                                 const std::string& host,
                                 const std::string& realm);

  void SetOffscreenPreRaster(bool enabled);

  void SetJavaPeers(JNIEnv* env,
                    const JavaParamRef<jobject>& web_contents_delegate,
                    const JavaParamRef<jobject>& contents_client_bridge,
                    const JavaParamRef<jobject>& io_thread_client,
                    const JavaParamRef<jobject>& intercept_navigation_delegate,
                    const JavaParamRef<jobject>& autofill_provider);
  base::android::ScopedJavaLocalRef<jobject> GetWebContents(JNIEnv* env);
  // base::android::ScopedJavaLocalRef<jobject> GetBrowserContext(
  //     JNIEnv* env,
  //     const base::android::JavaParamRef<jobject>& obj);
  // void SetCompositorFrameConsumer(
  //     JNIEnv* env,
  //     const base::android::JavaParamRef<jobject>& obj,
  //     jlong compositor_frame_consumer);
  base::android::ScopedJavaLocalRef<jobject> GetRenderProcess(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  void Destroy(JNIEnv* env);
  void DocumentHasImages(JNIEnv* env,
                         const base::android::JavaParamRef<jobject>& message);
  void GenerateMHTML(JNIEnv* env,
                     const base::android::JavaParamRef<jstring>& jpath,
                     const base::android::JavaParamRef<jobject>& callback);
  void CreatePdfExporter(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& pdfExporter);
  // void AddVisitedLinks(
  //     JNIEnv* env,
  //     const base::android::JavaParamRef<jobject>& obj,
  //     const base::android::JavaParamRef<jobjectArray>& jvisited_links);
  base::android::ScopedJavaLocalRef<jbyteArray> GetCertificate(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  void RequestNewHitTestDataAt(JNIEnv* env,
                               const base::android::JavaParamRef<jobject>&
                               obj, jfloat x, jfloat y, jfloat touch_major);
  void UpdateLastHitTestData(JNIEnv* env,
                             const base::android::JavaParamRef<jobject>&
                             obj);
  void OnSizeChanged(JNIEnv* env,
                     const base::android::JavaParamRef<jobject>& obj,
                     int w,
                     int h,
                     int ow,
                     int oh);
  void SetViewVisibility(JNIEnv* env,
                         const base::android::JavaParamRef<jobject>& obj,
                         bool visible);
  void SetWindowVisibility(JNIEnv* env,
                           const base::android::JavaParamRef<jobject>& obj,
                           bool visible);
  // void SetIsPaused(JNIEnv* env,
  //                  const base::android::JavaParamRef<jobject>& obj,
  //                  bool paused);
  void OnAttachedToWindow(JNIEnv* env,
                          const base::android::JavaParamRef<jobject>& obj,
                          int w,
                          int h);
  void OnDetachedFromWindow(JNIEnv* env,
                            const base::android::JavaParamRef<jobject>& obj);
  // bool IsVisible(JNIEnv* env, const base::android::JavaParamRef<jobject>&
  // obj);
  base::android::ScopedJavaLocalRef<jbyteArray> GetOpaqueState(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  jboolean RestoreFromOpaqueState(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jbyteArray>& state);
  void FocusFirstNode(JNIEnv* env,
                      const base::android::JavaParamRef<jobject>& obj);
  void SetBackgroundColor(JNIEnv* env,
                          const base::android::JavaParamRef<jobject>& obj,
                          jint color);
  void ZoomBy(JNIEnv* env,
              const base::android::JavaParamRef<jobject>& obj,
              jfloat delta);
  // void OnComputeScroll(JNIEnv* env,
  //                      const base::android::JavaParamRef<jobject>& obj,
  //                      jlong animation_time_millis);
  // bool OnDraw(JNIEnv* env,
  //             const base::android::JavaParamRef<jobject>& obj,
  //             const base::android::JavaParamRef<jobject>& canvas,
  //             jboolean is_hardware_accelerated,
  //             jint scroll_x,
  //             jint scroll_y,
  //             jint visible_left,
  //             jint visible_top,
  //             jint visible_right,
  //             jint visible_bottom,
  //             jboolean force_auxiliary_bitmap_rendering);
  // bool NeedToDrawBackgroundColor(
  //     JNIEnv* env,
  //     const base::android::JavaParamRef<jobject>& obj);
  // jlong CapturePicture(JNIEnv* env,
  //                      const base::android::JavaParamRef<jobject>& obj,
  //                      int width,
  //                      int height);
  // void EnableOnNewPicture(JNIEnv* env,
  //                         const base::android::JavaParamRef<jobject>& obj,
  //                         jboolean enabled);
  void InsertVisualStateCallback(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      jlong request_id,
      const base::android::JavaParamRef<jobject>& callback);
  // void ClearView(JNIEnv* env, const base::android::JavaParamRef<jobject>&
  // obj);
  void GrantFileSchemeAccesstoChildProcess(JNIEnv* env);
  void SetExtraHeadersForUrl(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& url,
      const base::android::JavaParamRef<jstring>& extra_headers);

  void InvokeGeolocationCallback(
      JNIEnv* env,
      jboolean value,
      const base::android::JavaParamRef<jstring>& origin);

  jint GetEffectivePriority(JNIEnv* env,
                          const base::android::JavaParamRef<jobject>& obj);

  js_injection::JsCommunicationHost* GetJsCommunicationHost();

  jint AddDocumentStartJavaScript(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& script,
      const base::android::JavaParamRef<jobjectArray>& allowed_origin_rules);

  void RemoveDocumentStartJavaScript(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      jint script_id);


  bool GetViewTreeForceDarkState() { return view_tree_force_dark_state_; }

  // PermissionRequestHandlerClient implementation.
  void OnPermissionRequest(base::android::ScopedJavaLocalRef<jobject> j_request,
                           BvPermissionRequest* request) override;
  void OnPermissionRequestCanceled(BvPermissionRequest* request) override;

  PermissionRequestHandler* GetPermissionRequestHandler() {
    return permission_request_handler_.get();
  }

  void PreauthorizePermission(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& origin,
      jlong resources);

  // BvBrowserPermissionRequestDelegate implementation.
  void RequestProtectedMediaIdentifierPermission(
      const GURL& origin,
      base::OnceCallback<void(bool)> callback) override;
  void CancelProtectedMediaIdentifierPermissionRequests(
      const GURL& origin) override;
  void RequestGeolocationPermission(
      const GURL& origin,
      base::OnceCallback<void(bool)> callback) override;
  void CancelGeolocationPermissionRequests(const GURL& origin) override;
  void RequestMIDISysexPermission(
      const GURL& origin,
      base::OnceCallback<void(bool)> callback) override;
  void CancelMIDISysexPermissionRequests(const GURL& origin) override;

  // Find-in-page API and related methods.
  void FindAllAsync(JNIEnv* env,
                    const base::android::JavaParamRef<jstring>& search_string);
  void FindNext(JNIEnv* env,
                jboolean forward);
  void ClearMatches(JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& obj);
  FindHelper* GetFindHelper();

  // Per WebView Cookie Policy
  bool AllowThirdPartyCookies();

  // FindHelper::Listener implementation.
  void OnFindResultReceived(int active_ordinal,
                            int match_count,
                            bool finished) override;
  // // IconHelper::Listener implementation.
  bool ShouldDownloadFavicon(const GURL& icon_url) override;
  void OnReceivedIcon(const GURL& icon_url, const SkBitmap& bitmap) override;
  void OnReceivedTouchIconUrl(const std::string& url,
                              const bool precomposed) override;

  // BisonRenderViewHostExtClient implementation.
  void OnWebLayoutPageScaleFactorChanged(float page_scale_factor) override;
  void OnWebLayoutContentsSizeChanged(const gfx::Size& contents_size) override;

  gfx::Point GetLocationOnScreen();
  void OnViewTreeForceDarkStateChanged(
      bool view_tree_force_dark_state);

  void ClearCache(JNIEnv* env,
                  jboolean include_disk_files);
  void KillRenderProcess(JNIEnv* env,
                         const base::android::JavaParamRef<jobject>& obj);
  void SetDipScale(JNIEnv* env,
                   const base::android::JavaParamRef<jobject>& obj,
                   jfloat dip_scale);
  void SetSaveFormData(bool enabled);

  // Sets the java client
  void SetAutofillClient(const base::android::JavaRef<jobject>& client);
  void SetJsOnlineProperty(JNIEnv* env,
                           const base::android::JavaParamRef<jobject>& obj,
                           jboolean network_up);
  jlong GetAutofillProvider(JNIEnv* env,
                            const base::android::JavaParamRef<jobject>& obj);

  void RendererUnresponsive(content::RenderProcessHost* render_process_host);
  void RendererResponsive(content::RenderProcessHost* render_process_host);

  // WebContentsObserver overrides
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;


  // BvRenderProcessGoneDelegate overrides
  RenderProcessGoneResult OnRenderProcessGone(int child_process_id,
                                              bool crashed) override;

  // jiang
  JavaObjectWeakGlobalRef java_ref_;

 private:
  void InitAutofillIfNecessary(bool autocomplete_enabled);
  void ShowGeolocationPrompt(const GURL& origin,
                             base::OnceCallback<void(bool)>);
  void HideGeolocationPrompt(const GURL& origin);



  std::unique_ptr<WebContents> web_contents_;
  std::unique_ptr<BvWebContentsDelegate> web_contents_delegate_;
  std::unique_ptr<BvContentsClientBridge> contents_client_bridge_;
  std::unique_ptr<BvRenderViewHostExt> render_view_host_ext_;
  std::unique_ptr<FindHelper> find_helper_;
  std::unique_ptr<BvPdfExporter> pdf_exporter_;
  std::unique_ptr<IconHelper> icon_helper_;
  std::unique_ptr<PermissionRequestHandler> permission_request_handler_;
  std::unique_ptr<autofill::AutofillProvider> autofill_provider_;
  std::unique_ptr<js_injection::JsCommunicationHost> js_communication_host_;

  bool view_tree_force_dark_state_ = false;

  // GURL is supplied by the content layer as requesting frame.
  // Callback is supplied by the content layer, and is invoked with the result
  // from the permission prompt.
  typedef std::pair<const GURL, base::OnceCallback<void(bool)>> OriginCallback;
  // The first element in the list is always the currently pending request.
  std::list<OriginCallback> pending_geolocation_prompts_;

  DISALLOW_COPY_AND_ASSIGN(BvContents);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_VIEW_H_
