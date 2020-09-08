// create by jiang947

#ifndef BISON_BROWSER_BISON_WEB_CONTENTS_DELEGATE_H_
#define BISON_BROWSER_BISON_WEB_CONTENTS_DELEGATE_H_

#include "components/embedder_support/android/delegate/web_contents_delegate_android.h"

using content::BluetoothChooser;
using content::BluetoothScanningPrompt;
using content::JavaScriptDialogManager;
using content::OpenURLParams;
using content::PictureInPictureResult;
using content::RenderFrameHost;
using content::RenderWidgetHost;
using content::WebContents;
using content::WebContentsDelegate;

namespace bison {

class BisonJavaScriptDialogManager;

class BisonWebContentsDelegate
    : public web_contents_delegate_android::WebContentsDelegateAndroid {
 public:
  BisonWebContentsDelegate(JNIEnv* env, jobject obj);
  ~BisonWebContentsDelegate() override;

  // WebContentsDelegate
  void AddNewContents(WebContents* source,
                      std::unique_ptr<WebContents> new_contents,
                      WindowOpenDisposition disposition,
                      const gfx::Rect& initial_rect,
                      bool user_gesture,
                      bool* was_blocked) override;
  void LoadingStateChanged(WebContents* source,
                           bool to_different_document) override;
  // void LoadProgressChanged(WebContents* source, double progress) override;
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
  JavaScriptDialogManager* GetJavaScriptDialogManager(
      WebContents* source) override;
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
  // std::unique_ptr<content::WebContents> SwapWebContents(
  //     content::WebContents* old_contents,
  //     std::unique_ptr<content::WebContents> new_contents,
  //     bool did_start_load,
  //     bool did_finish_load) override;
  bool ShouldAllowRunningInsecureContent(content::WebContents* web_contents,
                                         bool allowed_per_prefs,
                                         const url::Origin& origin,
                                         const GURL& resource_url) override;
  PictureInPictureResult EnterPictureInPicture(
      content::WebContents* web_contents,
      const viz::SurfaceId&,
      const gfx::Size& natural_size) override;
  bool ShouldResumeRequestsForCreatedWindow() override;

  void UpdateUserGestureCarryoverInfo(
      content::WebContents* web_contents) override;

 private:
  std::unique_ptr<BisonJavaScriptDialogManager> dialog_manager_;
  bool is_fullscreen_;
};

}  // namespace bison

// BisonWebContentsDelegate::BisonWebContentsDelegate(/* args */) {}

// BisonWebContentsDelegate::~BisonWebContentsDelegate() {}

#endif  // BISON_BROWSER_BISON_WEB_CONTENTS_DELEGATE_H_