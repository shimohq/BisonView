// create by jiang947

#ifndef BISON_BROWSER_BISON_VIEW_H_
#define BISON_BROWSER_BISON_VIEW_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "bison/browser/bison_browser_permission_request_delegate.h"
#include "bison/browser/permission/permission_request_handler_client.h"

#include "base/android/scoped_java_ref.h"
#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_piece.h"
#include "base/threading/thread_restrictions.h"
#include "bison/browser/renderer_host/bison_render_view_host_ext.h"
#include "build/build_config.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#include "content/public/browser/session_storage_namespace.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "ipc/ipc_channel.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/native_widget_types.h"

namespace content {
class BrowserContext;
class SiteInstance;
class WebContents;
class NavigationHandle;
}  // namespace content

class GURL;

using base::android::JavaParamRef;
using content::BluetoothChooser;
using content::BluetoothScanningPrompt;
using content::BrowserContext;
using content::JavaScriptDialogManager;
using content::NavigationEntry;
using content::OpenURLParams;
using content::PictureInPictureResult;
using content::RenderFrameHost;
using content::RenderWidgetHost;
using content::SessionStorageNamespace;
using content::SiteInstance;
using content::WebContents;
using content::WebContentsDelegate;
using content::WebContentsObserver;
using navigation_interception::InterceptNavigationDelegate;

namespace bison {

class BisonContentsClientBridge;
class BisonWebContentsDelegate;
class PermissionRequestHandler;

class BisonContents : public PermissionRequestHandlerClient,
                      public BisonBrowserPermissionRequestDelegate,
                      public WebContentsObserver {
 public:
  // Returns the BisonContents object corresponding to the given WebContents.
  static BisonContents* FromWebContents(WebContents* web_contents);
  static BisonContents* CreateBisonContents(BrowserContext* browser_context);

  static std::string GetLocale();

  static std::string GetLocaleList();

  BisonContents(std::unique_ptr<WebContents> web_contents);
  ~BisonContents() override;

  BisonRenderViewHostExt* render_view_host_ext() {
    return render_view_host_ext_.get();
  }

  WebContents* web_contents() const { return web_contents_.get(); }

  // jiang
  JavaObjectWeakGlobalRef java_ref_;
  base::android::ScopedJavaLocalRef<jobject> GetWebContents(JNIEnv* env);
  void SetJavaPeers(JNIEnv* env,
                    const JavaParamRef<jobject>& web_contents_delegate,
                    const JavaParamRef<jobject>& contents_client_bridge,
                    const JavaParamRef<jobject>& io_thread_client,
                    const JavaParamRef<jobject>& intercept_navigation_delegate);

  void GrantFileSchemeAccesstoChildProcess(JNIEnv* env);
  void SetExtraHeadersForUrl(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& url,
      const base::android::JavaParamRef<jstring>& extra_headers);
  void InvokeGeolocationCallback(
      JNIEnv* env,
      jboolean value,
      const base::android::JavaParamRef<jstring>& origin);
  // Per WebView Cookie Policy
  bool AllowThirdPartyCookies();

  void SetAutofillClient(const base::android::JavaRef<jobject>& client);

  void Destroy(JNIEnv* env);

 private:
  void ShowGeolocationPrompt(const GURL& origin,
                             base::OnceCallback<void(bool)>);
  void HideGeolocationPrompt(const GURL& origin);

  // WebContentsObserver overrides
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  // PermissionRequestHandlerClient implementation.
  void OnPermissionRequest(base::android::ScopedJavaLocalRef<jobject> j_request,
                           BisonPermissionRequest* request) override;
  void OnPermissionRequestCanceled(BisonPermissionRequest* request) override;

  PermissionRequestHandler* GetPermissionRequestHandler() {
    return permission_request_handler_.get();
  }

  // BisonBrowserPermissionRequestDelegate implementation.
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

  std::unique_ptr<WebContents> web_contents_;
  std::unique_ptr<BisonWebContentsDelegate> web_contents_delegate_;
  std::unique_ptr<BisonContentsClientBridge> contents_client_bridge_;
  std::unique_ptr<BisonRenderViewHostExt> render_view_host_ext_;

  std::unique_ptr<PermissionRequestHandler> permission_request_handler_;

  // base::ScopedAllowBlocking allow_blocking_;

  gfx::Size content_size_;

  // GURL is supplied by the content layer as requesting frame.
  // Callback is supplied by the content layer, and is invoked with the result
  // from the permission prompt.
  typedef std::pair<const GURL, base::OnceCallback<void(bool)>> OriginCallback;
  // The first element in the list is always the currently pending request.
  std::list<OriginCallback> pending_geolocation_prompts_;

  DISALLOW_COPY_AND_ASSIGN(BisonContents);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_VIEW_H_
