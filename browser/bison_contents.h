// create by jiang947

#ifndef BISON_BROWSER_BISON_VIEW_H_
#define BISON_BROWSER_BISON_VIEW_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/android/scoped_java_ref.h"
#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_piece.h"
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

class BisonDevToolsFrontend;
class BisonJavaScriptDialogManager;
class BisonContentsClientBridge;
class BisonWebContentsDelegate;

class BisonContents : public WebContentsDelegate, public WebContentsObserver {
 public:
  ~BisonContents() override;

  void LoadURL(const GURL& url);
  void LoadURLForFrame(const GURL& url,
                       const std::string& frame_name,
                       ui::PageTransition);
  void LoadDataWithBaseURL(const GURL& url,
                           const std::string& data,
                           const GURL& base_url);

  void LoadDataAsStringWithBaseURL(const GURL& url,
                                   const std::string& data,
                                   const GURL& base_url);

  void GoBackOrForward(int offset);
  void Reload();
  void ReloadBypassingCache();
  void Stop();
  void UpdateNavigationControls(bool to_different_document);
  void Close();
  void ShowDevTools();
  void CloseDevTools();

  static BisonContents* CreateNewWindow(
      BrowserContext* browser_context,
      const scoped_refptr<SiteInstance>& site_instance);

  // Returns the BisonContents object corresponding to the given WebContents.
  static BisonContents* FromWebContents(WebContents* web_contents);

  // Stores the supplied |quit_closure|, to be run when the last BisonContents
  // instance is destroyed.
  static void SetMainMessageLoopQuitClosure(base::OnceClosure quit_closure);

  // Used by the BlinkTestController to stop the message loop before closing all
  // windows, for specific tests. Fails if called after the message loop has
  // already been signalled to quit.
  static void QuitMainMessageLoopForTesting();

  BisonRenderViewHostExt* render_view_host_ext() {
    return render_view_host_ext_.get();
  }

  WebContents* web_contents() const { return web_contents_.get(); }

  // jiang
  base::android::ScopedJavaGlobalRef<jobject> java_object_;
  base::android::ScopedJavaLocalRef<jobject> GetWebContents(JNIEnv* env);
  void SetJavaPeers(JNIEnv* env,
                    const JavaParamRef<jobject>& web_contents_delegate,
                    const JavaParamRef<jobject>& contents_client_bridge,
                    const JavaParamRef<jobject>& intercept_navigation_delegate);

  void GrantFileSchemeAccesstoChildProcess(JNIEnv* env);

  void Destroy(JNIEnv* env);
  // const JavaParamRef<jobject>& intercept_navigation_delegate

 private:
  class DevToolsWebContentsObserver;

  BisonContents(std::unique_ptr<WebContents> web_contents);

  // Helper to create a new Shell given a newly created WebContents.
  static BisonContents* CreateBisonContents(
      std::unique_ptr<WebContents> web_contents);

  // Helper for one time deinitialization of platform specific state.
  static void PlatformExit();

  // All the methods that begin with Platform need to be implemented by the
  // platform specific Shell implementation.
  // Called from the destructor to let each platform do any necessary cleanup.
  void PlatformCleanUp();
  // Creates the main window GUI.
  void PlatformCreateWindow();

  // void PlatformToggleFullscreenModeForTab(WebContents* web_contents,
  //                                         bool enter_fullscreen);
  bool PlatformIsFullscreenForTabOrPending(
      const WebContents* web_contents) const;

  // Helper method for the two public LoadData methods.
  void LoadDataWithBaseURLInternal(const GURL& url,
                                   const std::string& data,
                                   const GURL& base_url,
                                   bool load_as_string);

  gfx::NativeView GetContentView();

  void ToggleFullscreenModeForTab(WebContents* web_contents,
                                  bool enter_fullscreen);

  // WebContentsObserver overrides
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  void OnDevToolsWebContentsDestroyed();

  std::unique_ptr<BisonJavaScriptDialogManager> dialog_manager_;

  std::unique_ptr<WebContents> web_contents_;
  std::unique_ptr<BisonWebContentsDelegate> web_contents_delegate_;
  std::unique_ptr<BisonContentsClientBridge> contents_client_bridge_;
  std::unique_ptr<BisonRenderViewHostExt> render_view_host_ext_;
  std::unique_ptr<DevToolsWebContentsObserver> devtools_observer_;
  BisonDevToolsFrontend* devtools_frontend_;

  bool is_fullscreen_;

  gfx::Size content_size_;

  bool headless_;
  bool delay_popup_contents_delegate_for_testing_ = false;

  // A container of all the open windows. We use a vector so we can keep track
  // of ordering.
  static std::vector<BisonContents*> windows_;

  // static base::OnceCallback<void(BisonContents*)>
  // bison_view_created_callback_;
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_VIEW_H_
