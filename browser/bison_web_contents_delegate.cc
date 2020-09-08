#include "bison_web_contents_delegate.h"

#include "bison_javascript_dialog_manager.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"

using content::NavigationController;

namespace bison {

BisonWebContentsDelegate::BisonWebContentsDelegate(JNIEnv* env, jobject obj)
    : WebContentsDelegateAndroid(env, obj), is_fullscreen_(false) {}

BisonWebContentsDelegate::~BisonWebContentsDelegate() {}

void BisonWebContentsDelegate::AddNewContents(
    WebContents* source,
    std::unique_ptr<WebContents> new_contents,
    WindowOpenDisposition disposition,
    const gfx::Rect& initial_rect,
    bool user_gesture,
    bool* was_blocked) {
  VLOG(0) << "AddNewContents";
}

void BisonWebContentsDelegate::LoadingStateChanged(WebContents* source,
                                                   bool to_different_document) {

}

// void BisonWebContentsDelegate::LoadProgressChanged(WebContents* source,
//                                                    double progress) {
// JNIEnv* env = AttachCurrentThread();
// Java_BisonWebContentsDelegate_onLoadProgressChanged(env, java_object_,
//                                                     progress);
// }

void BisonWebContentsDelegate::SetOverlayMode(bool use_overlay_mode) {
  // JNIEnv* env = base::android::AttachCurrentThread();
  // return Java_BisonWebContentsDelegate_setOverlayMode(env, java_object_,
  //                                                     use_overlay_mode);
}

void BisonWebContentsDelegate::EnterFullscreenModeForTab(
    WebContents* web_contents,
    const GURL& origin,
    const blink::mojom::FullscreenOptions& options) {
  // ToggleFullscreenModeForTab(web_contents, true);
}

void BisonWebContentsDelegate::ExitFullscreenModeForTab(
    WebContents* web_contents) {
  // ToggleFullscreenModeForTab(web_contents, false);
}

bool BisonWebContentsDelegate::IsFullscreenForTabOrPending(
    const WebContents* web_contents) {
  // #if defined(OS_ANDROID)
  //   return PlatformIsFullscreenForTabOrPending(web_contents);
  // #else
  //   return is_fullscreen_;
  // #endif
  return is_fullscreen_;
}

blink::mojom::DisplayMode BisonWebContentsDelegate::GetDisplayMode(
    const WebContents* web_contents) {
  // TODO: should return blink::mojom::DisplayModeFullscreen wherever user puts
  // a browser window into fullscreen (not only in case of renderer-initiated
  // fullscreen mode): crbug.com/476874.
  return IsFullscreenForTabOrPending(web_contents)
             ? blink::mojom::DisplayMode::kFullscreen
             : blink::mojom::DisplayMode::kBrowser;
}

void BisonWebContentsDelegate::RequestToLockMouse(
    WebContents* web_contents,
    bool user_gesture,
    bool last_unlocked_by_target) {
  web_contents->GotResponseToLockMouseRequest(true);
}

void BisonWebContentsDelegate::CloseContents(WebContents* source) {
  // Close();
}

bool BisonWebContentsDelegate::CanOverscrollContent() {
  return false;
}

void BisonWebContentsDelegate::DidNavigateMainFramePostCommit(
    WebContents* web_contents) {}

JavaScriptDialogManager* BisonWebContentsDelegate::GetJavaScriptDialogManager(
    WebContents* source) {
  if (!dialog_manager_) {
    dialog_manager_.reset(new BisonJavaScriptDialogManager);
  }
  return dialog_manager_.get();
}

std::unique_ptr<BluetoothChooser> BisonWebContentsDelegate::RunBluetoothChooser(
    RenderFrameHost* frame,
    const BluetoothChooser::EventHandler& event_handler) {
  // BlinkTestController* blink_test_controller = BlinkTestController::Get();
  // if (blink_test_controller && switches::IsRunWebTestsSwitchPresent())
  //   return blink_test_controller->RunBluetoothChooser(frame, event_handler);
  return nullptr;
}

std::unique_ptr<BluetoothScanningPrompt>
BisonWebContentsDelegate::ShowBluetoothScanningPrompt(
    RenderFrameHost* frame,
    const BluetoothScanningPrompt::EventHandler& event_handler) {
  // return std::make_unique<FakeBluetoothScanningPrompt>(event_handler);
  return nullptr;
}

bool BisonWebContentsDelegate::DidAddMessageToConsole(
    WebContents* source,
    blink::mojom::ConsoleMessageLevel log_level,
    const base::string16& message,
    int32_t line_no,
    const base::string16& source_id) {
  return false;
}

void BisonWebContentsDelegate::PortalWebContentsCreated(
    WebContents* portal_web_contents) {}

void BisonWebContentsDelegate::RendererUnresponsive(
    WebContents* source,
    RenderWidgetHost* render_widget_host,
    base::RepeatingClosure hang_monitor_restarter) {
  // BlinkTestController* blink_test_controller = BlinkTestController::Get();
  // if (blink_test_controller && switches::IsRunWebTestsSwitchPresent())
  //   blink_test_controller->RendererUnresponsive();
}

void BisonWebContentsDelegate::ActivateContents(WebContents* contents) {
  contents->GetRenderViewHost()->GetWidget()->Focus();
}

bool BisonWebContentsDelegate::ShouldAllowRunningInsecureContent(
    WebContents* web_contents,
    bool allowed_per_prefs,
    const url::Origin& origin,
    const GURL& resource_url) {
  // bool allowed_by_test = false;
  // BlinkTestController* blink_test_controller = BlinkTestController::Get();
  // if (blink_test_controller && switches::IsRunWebTestsSwitchPresent()) {
  //   const base::DictionaryValue& test_flags =
  //       blink_test_controller->accumulated_web_test_runtime_flags_changes();
  //   test_flags.GetBoolean("running_insecure_content_allowed",
  //   &allowed_by_test);
  // }

  return allowed_per_prefs;
}

PictureInPictureResult BisonWebContentsDelegate::EnterPictureInPicture(
    WebContents* web_contents,
    const viz::SurfaceId& surface_id,
    const gfx::Size& natural_size) {
  // During tests, returning success to pretend the window was created and allow
  // tests to run accordingly.
  return PictureInPictureResult::kSuccess;
}

bool BisonWebContentsDelegate::ShouldResumeRequestsForCreatedWindow() {
  return false;
}

void BisonWebContentsDelegate::UpdateUserGestureCarryoverInfo(
    content::WebContents* web_contents) {
  auto* intercept_navigation_delegate =
      navigation_interception::InterceptNavigationDelegate::Get(web_contents);
  if (intercept_navigation_delegate)
    intercept_navigation_delegate->UpdateLastUserGestureCarryoverTimestamp();
}

}  // namespace bison