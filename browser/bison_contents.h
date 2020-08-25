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
#include "build/build_config.h"
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

using content::BluetoothChooser;
using content::BluetoothScanningPrompt;
using content::BrowserContext;
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

namespace bison {

class BisonDevToolsFrontend;
// class ShellJavaScriptDialogManager;

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

  // Resizes the web content view to the given dimensions.
  void SizeTo(const gfx::Size& content_size);

  static BisonContents* CreateNewWindow(
      BrowserContext* browser_context,
      const scoped_refptr<SiteInstance>& site_instance);

  static BisonContents* CreateNewWindowWithSessionStorageNamespace(
      BrowserContext* browser_context,
      const GURL& url,
      const scoped_refptr<SiteInstance>& site_instance,
      const gfx::Size& initial_size,
      scoped_refptr<SessionStorageNamespace> session_storage_namespace);

  // Returns the BisonContents object corresponding to the given WebContents.
  static BisonContents* FromWebContents(WebContents* web_contents);

  // Returns the currently open windows.
  static std::vector<BisonContents*>& windows() { return windows_; }

  // Closes all windows, pumps teardown tasks, then returns. The main message
  // loop will be signalled to quit, before the call returns.
  static void CloseAllWindows();

  // Stores the supplied |quit_closure|, to be run when the last BisonContents
  // instance is destroyed.
  static void SetMainMessageLoopQuitClosure(base::OnceClosure quit_closure);

  // Used by the BlinkTestController to stop the message loop before closing all
  // windows, for specific tests. Fails if called after the message loop has
  // already been signalled to quit.
  static void QuitMainMessageLoopForTesting();

  // Used for content_browsertests. Called once.
  static void SetBisonContentsCreatedCallback(
      base::OnceCallback<void(BisonContents*)> bison_view_created_callback);

  WebContents* web_contents() const { return web_contents_.get(); }
  gfx::NativeWindow window() { return window_; }

  // WebContentsDelegate
  WebContents* OpenURLFromTab(WebContents* source,
                              const OpenURLParams& params) override;
  void AddNewContents(WebContents* source,
                      std::unique_ptr<WebContents> new_contents,
                      WindowOpenDisposition disposition,
                      const gfx::Rect& initial_rect,
                      bool user_gesture,
                      bool* was_blocked) override;
  void LoadingStateChanged(WebContents* source,
                           bool to_different_document) override;

  void LoadProgressChanged(WebContents* source, double progress) override;
  void SetOverlayMode(bool use_overlay_mode) override;

  void EnterFullscreenModeForTab(
      WebContents* web_contents,
      const GURL& origin,
      const blink::mojom::FullscreenOptions& options) override;
  void ExitFullscreenModeForTab(WebContents* web_contents) override;
  bool IsFullscreenForTabOrPending(const WebContents* web_contents) override;
  blink::mojom::DisplayMode GetDisplayMode(
      const WebContents* web_contents) override;
  void RequestToLockMouse(WebContents* web_contents,
                          bool user_gesture,
                          bool last_unlocked_by_target) override;
  void CloseContents(WebContents* source) override;
  bool CanOverscrollContent() override;
  void DidNavigateMainFramePostCommit(WebContents* web_contents) override;
  // JavaScriptDialogManager* GetJavaScriptDialogManager(
  //     WebContents* source) override;
  std::unique_ptr<BluetoothChooser> RunBluetoothChooser(
      RenderFrameHost* frame,
      const BluetoothChooser::EventHandler& event_handler) override;
  std::unique_ptr<BluetoothScanningPrompt> ShowBluetoothScanningPrompt(
      RenderFrameHost* frame,
      const BluetoothScanningPrompt::EventHandler& event_handler) override;
  bool DidAddMessageToConsole(WebContents* source,
                              blink::mojom::ConsoleMessageLevel log_level,
                              const base::string16& message,
                              int32_t line_no,
                              const base::string16& source_id) override;
  void PortalWebContentsCreated(WebContents* portal_web_contents) override;
  void RendererUnresponsive(
      WebContents* source,
      RenderWidgetHost* render_widget_host,
      base::RepeatingClosure hang_monitor_restarter) override;
  void ActivateContents(WebContents* contents) override;
  std::unique_ptr<content::WebContents> SwapWebContents(
      content::WebContents* old_contents,
      std::unique_ptr<content::WebContents> new_contents,
      bool did_start_load,
      bool did_finish_load) override;
  bool ShouldAllowRunningInsecureContent(content::WebContents* web_contents,
                                         bool allowed_per_prefs,
                                         const url::Origin& origin,
                                         const GURL& resource_url) override;
  PictureInPictureResult EnterPictureInPicture(
      content::WebContents* web_contents,
      const viz::SurfaceId&,
      const gfx::Size& natural_size) override;
  bool ShouldResumeRequestsForCreatedWindow() override;

  void set_delay_popup_contents_delegate_for_testing(bool delay) {
    delay_popup_contents_delegate_for_testing_ = delay;
  }

  base::android::ScopedJavaGlobalRef<jobject> java_object_;
  base::android::ScopedJavaLocalRef<jobject> GetWebContents(JNIEnv* env);

 private:
  enum UIControl { BACK_BUTTON, FORWARD_BUTTON, STOP_BUTTON };

  class DevToolsWebContentsObserver;

  BisonContents(std::unique_ptr<WebContents> web_contents,
                bool should_set_delegate);

  // Helper to create a new Shell given a newly created WebContents.
  static BisonContents* CreateBisonContents(
      std::unique_ptr<WebContents> web_contents,
      bool should_set_delegate);

  // Helper for one time initialization of application
  static void PlatformInitialize(const gfx::Size& default_window_size);
  // Helper for one time deinitialization of platform specific state.
  static void PlatformExit();

  // All the methods that begin with Platform need to be implemented by the
  // platform specific Shell implementation.
  // Called from the destructor to let each platform do any necessary cleanup.
  void PlatformCleanUp();
  // Creates the main window GUI.
  void PlatformCreateWindow();
  // Links the WebContents into the newly created window.
  void PlatformSetContents();
  // Resize the content area and GUI.
  void PlatformResizeSubViews();
  // Enable/disable a button.
  void PlatformEnableUIControl(UIControl control, bool is_enabled);
  // Updates the url in the url bar.
  void PlatformSetAddressBarURL(const GURL& url);
  // Sets whether the spinner is spinning.
  void PlatformSetIsLoading(bool loading);
  // Set the title of shell window
  void PlatformSetTitle(const base::string16& title);

  void PlatformToggleFullscreenModeForTab(WebContents* web_contents,
                                          bool enter_fullscreen);
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

  void TitleWasSet(NavigationEntry* entry) override;

  void OnDevToolsWebContentsDestroyed();

  // std::unique_ptr<ShellJavaScriptDialogManager> dialog_manager_;

  std::unique_ptr<WebContents> web_contents_;

  std::unique_ptr<DevToolsWebContentsObserver> devtools_observer_;
  BisonDevToolsFrontend* devtools_frontend_;

  bool is_fullscreen_;

  gfx::NativeWindow window_;

  gfx::Size content_size_;

  bool headless_;
  bool delay_popup_contents_delegate_for_testing_ = false;

  // A container of all the open windows. We use a vector so we can keep track
  // of ordering.
  static std::vector<BisonContents*> windows_;

  static base::OnceCallback<void(BisonContents*)> bison_view_created_callback_;
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_VIEW_H_
