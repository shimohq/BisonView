#include "bison_contents.h"

#include <stddef.h>

#include <map>
#include <string>
#include <utility>

#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/no_destructor.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
// jiang jni.h
#include "bison/bison_jni_headers/BisonContents_jni.h"
#include "bison_browser_main_parts.h"
#include "bison_content_browser_client.h"
#include "bison_devtools_frontend.h"
// #include "bison_view_manager.h"
// #include "bison_javascript_dialog_manager.h"
// #include "bison_switches.h"
#include "build/build_config.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/picture_in_picture_window_controller.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "media/media_buildflags.h"
#include "third_party/blink/public/common/peerconnection/webrtc_ip_handling_policy.h"
#include "third_party/blink/public/common/presentation/presentation_receiver_flags.h"
#include "third_party/blink/public/mojom/renderer_preferences.mojom.h"

using base::android::AttachCurrentThread;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using content::BluetoothChooser;
using content::BluetoothScanningPrompt;
using content::DevToolsAgentHost;
using content::NavigationController;
using content::NavigationEntry;
using content::PictureInPictureResult;
using content::ReloadType;
using content::RenderFrameHost;
using content::RenderProcessHost;
using content::RenderWidgetHost;

namespace bison {

// Null until/unless the default main message loop is running.
base::NoDestructor<base::OnceClosure> g_quit_main_message_loop;

std::vector<BisonContents*> BisonContents::windows_;
base::OnceCallback<void(BisonContents*)>
    BisonContents::bison_view_created_callback_;

class BisonContents::DevToolsWebContentsObserver : public WebContentsObserver {
 public:
  DevToolsWebContentsObserver(BisonContents* bison_view,
                              WebContents* web_contents)
      : WebContentsObserver(web_contents), bison_view_(bison_view) {}

  // WebContentsObserver
  void WebContentsDestroyed() override {
    bison_view_->OnDevToolsWebContentsDestroyed();
  }

 private:
  BisonContents* bison_view_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsWebContentsObserver);
};

BisonContents::BisonContents(std::unique_ptr<WebContents> web_contents,
                             bool should_set_delegate)
    : WebContentsObserver(web_contents.get()),
      web_contents_(std::move(web_contents)),
      devtools_frontend_(nullptr),
      is_fullscreen_(false),
      window_(nullptr),
      headless_(false) {
  VLOG(0) << "create: should_set_delegate" << should_set_delegate;
  if (should_set_delegate)
    web_contents_->SetDelegate(this);

  windows_.push_back(this);

  if (bison_view_created_callback_)
    std::move(bison_view_created_callback_).Run(this);
}

BisonContents::~BisonContents() {
  PlatformCleanUp();

  for (size_t i = 0; i < windows_.size(); ++i) {
    if (windows_[i] == this) {
      windows_.erase(windows_.begin() + i);
      break;
    }
  }

  // Always destroy WebContents before calling PlatformExit(). WebContents
  // destruction sequence may depend on the resources destroyed in
  // PlatformExit() (e.g. the display::Screen singleton).
  web_contents_->SetDelegate(nullptr);
  web_contents_.reset();

  if (windows_.empty()) {
    if (headless_)
      PlatformExit();
    for (auto it = RenderProcessHost::AllHostsIterator(); !it.IsAtEnd();
         it.Advance()) {
      it.GetCurrentValue()->DisableKeepAliveRefCount();
    }
    if (*g_quit_main_message_loop)
      std::move(*g_quit_main_message_loop).Run();
  }
}

BisonContents* BisonContents::CreateBisonContents(
    std::unique_ptr<WebContents> web_contents,
    bool should_set_delegate) {
  // WebContents* raw_web_contents = web_contents.get();
  BisonContents* bison_view =
      new BisonContents(std::move(web_contents), should_set_delegate);
  // bison_view->PlatformCreateWindow();

  // bison_view->PlatformSetContents();

  // bison_view->PlatformResizeSubViews();

  return bison_view;
}

void BisonContents::CloseAllWindows() {
  DevToolsAgentHost::DetachAllClients();
  std::vector<BisonContents*> open_windows(windows_);
  for (size_t i = 0; i < open_windows.size(); ++i)
    open_windows[i]->Close();

  // Pump the message loop to allow window teardown tasks to run.
  base::RunLoop().RunUntilIdle();

  // If there were no windows open then the message loop quit closure will
  // not have been run.
  if (*g_quit_main_message_loop)
    std::move(*g_quit_main_message_loop).Run();

  PlatformExit();
}

void BisonContents::SetMainMessageLoopQuitClosure(
    base::OnceClosure quit_closure) {
  *g_quit_main_message_loop = std::move(quit_closure);
}

void BisonContents::QuitMainMessageLoopForTesting() {
  DCHECK(*g_quit_main_message_loop);
  std::move(*g_quit_main_message_loop).Run();
}

void BisonContents::SetBisonContentsCreatedCallback(
    base::OnceCallback<void(BisonContents*)> bison_view_created_callback) {
  DCHECK(!bison_view_created_callback_);
  bison_view_created_callback_ = std::move(bison_view_created_callback);
}

BisonContents* BisonContents::FromWebContents(WebContents* web_contents) {
  for (BisonContents* window : windows_) {
    if (window->web_contents() && window->web_contents() == web_contents) {
      return window;
    }
  }
  return nullptr;
}

BisonContents* BisonContents::CreateNewWindow(
    BrowserContext* browser_context,
    const scoped_refptr<SiteInstance>& site_instance) {
  WebContents::CreateParams create_params(browser_context, site_instance);
  std::unique_ptr<WebContents> web_contents =
      WebContents::Create(create_params);
  BisonContents* bison_view = CreateBisonContents(
      std::move(web_contents), true /* should_set_delegate */);

  return bison_view;
}

void BisonContents::LoadURL(const GURL& url) {
  LoadURLForFrame(
      url, std::string(),
      ui::PageTransitionFromInt(ui::PAGE_TRANSITION_TYPED |
                                ui::PAGE_TRANSITION_FROM_ADDRESS_BAR));
}

void BisonContents::LoadURLForFrame(const GURL& url,
                                    const std::string& frame_name,
                                    ui::PageTransition transition_type) {
  NavigationController::LoadURLParams params(url);
  params.frame_name = frame_name;
  params.transition_type = transition_type;
  web_contents_->GetController().LoadURLWithParams(params);
  web_contents_->Focus();
}

void BisonContents::LoadDataWithBaseURL(const GURL& url,
                                        const std::string& data,
                                        const GURL& base_url) {
  bool load_as_string = false;
  LoadDataWithBaseURLInternal(url, data, base_url, load_as_string);
}

void BisonContents::LoadDataAsStringWithBaseURL(const GURL& url,
                                                const std::string& data,
                                                const GURL& base_url) {
  bool load_as_string = true;
  LoadDataWithBaseURLInternal(url, data, base_url, load_as_string);
}

void BisonContents::LoadDataWithBaseURLInternal(const GURL& url,
                                                const std::string& data,
                                                const GURL& base_url,
                                                bool load_as_string) {
  NavigationController::LoadURLParams params(GURL::EmptyGURL());
  const std::string data_url_header = "data:text/html;charset=utf-8,";
  if (load_as_string) {
    params.url = GURL(data_url_header);
    std::string data_url_as_string = data_url_header + data;
    params.data_url_as_string =
        base::RefCountedString::TakeString(&data_url_as_string);
  } else {
    params.url = GURL(data_url_header + data);
  }

  params.load_type = NavigationController::LOAD_TYPE_DATA;
  params.base_url_for_data_url = base_url;
  params.virtual_url_for_data_url = url;
  params.override_user_agent = NavigationController::UA_OVERRIDE_FALSE;
  web_contents_->GetController().LoadURLWithParams(params);
  web_contents_->Focus();
}

void BisonContents::AddNewContents(WebContents* source,
                                   std::unique_ptr<WebContents> new_contents,
                                   WindowOpenDisposition disposition,
                                   const gfx::Rect& initial_rect,
                                   bool user_gesture,
                                   bool* was_blocked) {
  VLOG(0) << "AddNewContents";
}

void BisonContents::GoBackOrForward(int offset) {
  web_contents_->GetController().GoToOffset(offset);
  web_contents_->Focus();
}

void BisonContents::Reload() {
  web_contents_->GetController().Reload(ReloadType::NORMAL, false);
  web_contents_->Focus();
}

void BisonContents::ReloadBypassingCache() {
  web_contents_->GetController().Reload(ReloadType::BYPASSING_CACHE, false);
  web_contents_->Focus();
}

void BisonContents::Stop() {
  web_contents_->Stop();
  web_contents_->Focus();
}

void BisonContents::UpdateNavigationControls(bool to_different_document) {
  int current_index = web_contents_->GetController().GetCurrentEntryIndex();
  int max_index = web_contents_->GetController().GetEntryCount() - 1;

  PlatformEnableUIControl(BACK_BUTTON, current_index > 0);
  PlatformEnableUIControl(FORWARD_BUTTON, current_index < max_index);
  PlatformEnableUIControl(STOP_BUTTON,
                          to_different_document && web_contents_->IsLoading());
}

void BisonContents::ShowDevTools() {
  if (!devtools_frontend_) {
    devtools_frontend_ = BisonDevToolsFrontend::Show(web_contents());
    devtools_observer_.reset(new DevToolsWebContentsObserver(
        this, devtools_frontend_->frontend_shell()->web_contents()));
  }

  devtools_frontend_->Activate();
  devtools_frontend_->Focus();
}

void BisonContents::CloseDevTools() {
  if (!devtools_frontend_)
    return;
  devtools_observer_.reset();
  devtools_frontend_->Close();
  devtools_frontend_ = nullptr;
}

gfx::NativeView BisonContents::GetContentView() {
  if (!web_contents_)
    return nullptr;
  return web_contents_->GetNativeView();
}

WebContents* BisonContents::OpenURLFromTab(WebContents* source,
                                           const OpenURLParams& params) {
  VLOG(0) << "OpenURLFromTab params";
  WebContents* target = nullptr;
  switch (params.disposition) {
    case WindowOpenDisposition::CURRENT_TAB:
      target = source;
      break;

    // Normally, the difference between NEW_POPUP and NEW_WINDOW is that a popup
    // should have no toolbar, no status bar, no menu bar, no scrollbars and be
    // not resizable.  For simplicity and to enable new testing scenarios in
    // content bison_view and web tests, popups don't get special treatment
    // below (i.e. they will have a toolbar and other things described here).
    case WindowOpenDisposition::NEW_POPUP:
    case WindowOpenDisposition::NEW_WINDOW:
    // content_shell doesn't really support tabs, but some web tests use
    // middle click (which translates into kNavigationPolicyNewBackgroundTab),
    // so we treat the cases below just like a NEW_WINDOW disposition.
    case WindowOpenDisposition::NEW_BACKGROUND_TAB:
    case WindowOpenDisposition::NEW_FOREGROUND_TAB: {
      break;
    }

    // No tabs in content_shell:
    case WindowOpenDisposition::SINGLETON_TAB:
    // No incognito mode in content_shell:
    case WindowOpenDisposition::OFF_THE_RECORD:
    // TODO(lukasza): Investigate if some web tests might need support for
    // SAVE_TO_DISK disposition.  This would probably require that
    // BlinkTestController always sets up and cleans up a temporary directory
    // as the default downloads destinations for the duration of a test.
    case WindowOpenDisposition::SAVE_TO_DISK:
    // Ignoring requests with disposition == IGNORE_ACTION...
    case WindowOpenDisposition::IGNORE_ACTION:
    default:
      return nullptr;
  }

  target->GetController().LoadURLWithParams(
      NavigationController::LoadURLParams(params));
  return target;
}

void BisonContents::LoadingStateChanged(WebContents* source,
                                        bool to_different_document) {
  UpdateNavigationControls(to_different_document);
  PlatformSetIsLoading(source->IsLoading());
}

void BisonContents::EnterFullscreenModeForTab(
    WebContents* web_contents,
    const GURL& origin,
    const blink::mojom::FullscreenOptions& options) {
  ToggleFullscreenModeForTab(web_contents, true);
}

void BisonContents::ExitFullscreenModeForTab(WebContents* web_contents) {
  ToggleFullscreenModeForTab(web_contents, false);
}

void BisonContents::ToggleFullscreenModeForTab(WebContents* web_contents,
                                               bool enter_fullscreen) {
  PlatformToggleFullscreenModeForTab(web_contents, enter_fullscreen);
  if (is_fullscreen_ != enter_fullscreen) {
    is_fullscreen_ = enter_fullscreen;
    web_contents->GetRenderViewHost()
        ->GetWidget()
        ->SynchronizeVisualProperties();
  }
}

bool BisonContents::IsFullscreenForTabOrPending(
    const WebContents* web_contents) {
#if defined(OS_ANDROID)
  return PlatformIsFullscreenForTabOrPending(web_contents);
#else
  return is_fullscreen_;
#endif
}

blink::mojom::DisplayMode BisonContents::GetDisplayMode(
    const WebContents* web_contents) {
  // TODO: should return blink::mojom::DisplayModeFullscreen wherever user puts
  // a browser window into fullscreen (not only in case of renderer-initiated
  // fullscreen mode): crbug.com/476874.
  return IsFullscreenForTabOrPending(web_contents)
             ? blink::mojom::DisplayMode::kFullscreen
             : blink::mojom::DisplayMode::kBrowser;
}

void BisonContents::RequestToLockMouse(WebContents* web_contents,
                                       bool user_gesture,
                                       bool last_unlocked_by_target) {
  web_contents->GotResponseToLockMouseRequest(true);
}

void BisonContents::CloseContents(WebContents* source) {
  Close();
}

bool BisonContents::CanOverscrollContent() {
  return false;
}

void BisonContents::DidNavigateMainFramePostCommit(WebContents* web_contents) {
  PlatformSetAddressBarURL(web_contents->GetVisibleURL());
}

// JavaScriptDialogManager* BisonContents::GetJavaScriptDialogManager(
//     WebContents* source) {
//   if (!dialog_manager_) {
//     dialog_manager_.reset(new ShellJavaScriptDialogManager);
//   }
//   return dialog_manager_.get();
// }

std::unique_ptr<BluetoothChooser> BisonContents::RunBluetoothChooser(
    RenderFrameHost* frame,
    const BluetoothChooser::EventHandler& event_handler) {
  // BlinkTestController* blink_test_controller = BlinkTestController::Get();
  // if (blink_test_controller && switches::IsRunWebTestsSwitchPresent())
  //   return blink_test_controller->RunBluetoothChooser(frame, event_handler);
  return nullptr;
}

std::unique_ptr<BluetoothScanningPrompt>
BisonContents::ShowBluetoothScanningPrompt(
    RenderFrameHost* frame,
    const BluetoothScanningPrompt::EventHandler& event_handler) {
  // return std::make_unique<FakeBluetoothScanningPrompt>(event_handler);
  return nullptr;
}

bool BisonContents::DidAddMessageToConsole(
    WebContents* source,
    blink::mojom::ConsoleMessageLevel log_level,
    const base::string16& message,
    int32_t line_no,
    const base::string16& source_id) {
  return false;
}

void BisonContents::PortalWebContentsCreated(WebContents* portal_web_contents) {
}

void BisonContents::RendererUnresponsive(
    WebContents* source,
    RenderWidgetHost* render_widget_host,
    base::RepeatingClosure hang_monitor_restarter) {
  // BlinkTestController* blink_test_controller = BlinkTestController::Get();
  // if (blink_test_controller && switches::IsRunWebTestsSwitchPresent())
  //   blink_test_controller->RendererUnresponsive();
}

void BisonContents::ActivateContents(WebContents* contents) {
  contents->GetRenderViewHost()->GetWidget()->Focus();
}

std::unique_ptr<WebContents> BisonContents::SwapWebContents(
    WebContents* old_contents,
    std::unique_ptr<WebContents> new_contents,
    bool did_start_load,
    bool did_finish_load) {
  DCHECK_EQ(old_contents, web_contents_.get());
  new_contents->SetDelegate(this);
  web_contents_->SetDelegate(nullptr);
  VLOG(0) << "SwapWebContents";
  for (auto* bison_view_devtools_bindings :
       BisonDevToolsBindings::GetInstancesForWebContents(old_contents)) {
    bison_view_devtools_bindings->UpdateInspectedWebContents(
        new_contents.get());
  }
  std::swap(web_contents_, new_contents);
  PlatformSetContents();
  PlatformSetAddressBarURL(web_contents_->GetVisibleURL());
  LoadingStateChanged(web_contents_.get(), true);
  return new_contents;
}

bool BisonContents::ShouldAllowRunningInsecureContent(
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

PictureInPictureResult BisonContents::EnterPictureInPicture(
    WebContents* web_contents,
    const viz::SurfaceId& surface_id,
    const gfx::Size& natural_size) {
  // During tests, returning success to pretend the window was created and allow
  // tests to run accordingly.
  return PictureInPictureResult::kSuccess;
}

bool BisonContents::ShouldResumeRequestsForCreatedWindow() {
  return !delay_popup_contents_delegate_for_testing_;
}

void BisonContents::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  net::Error error_code = navigation_handle->GetNetErrorCode();
  if (error_code != net::ERR_BLOCKED_BY_CLIENT &&
      error_code != net::ERR_BLOCKED_BY_ADMINISTRATOR &&
      error_code != net::ERR_ABORTED) {
    return;
  }
  VLOG(0) << "DidFinishNavigation";
}

void BisonContents::TitleWasSet(NavigationEntry* entry) {
  if (entry)
    PlatformSetTitle(entry->GetTitle());
}

void BisonContents::OnDevToolsWebContentsDestroyed() {
  devtools_observer_.reset();
  devtools_frontend_ = nullptr;
}

void BisonContents::PlatformInitialize(const gfx::Size& default_window_size) {}

void BisonContents::PlatformExit() {
  VLOG(0) << "Destroy";
}

void BisonContents::PlatformCleanUp() {
  JNIEnv* env = AttachCurrentThread();
  if (java_object_.is_null())
    return;
  Java_BisonContents_onNativeDestroyed(env, java_object_);
}

void BisonContents::PlatformEnableUIControl(UIControl control,
                                            bool is_enabled) {
  JNIEnv* env = AttachCurrentThread();
  if (java_object_.is_null())
    return;
  Java_BisonContents_enableUiControl(env, java_object_, control, is_enabled);
}

void BisonContents::PlatformSetAddressBarURL(const GURL& url) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> j_url = ConvertUTF8ToJavaString(env, url.spec());
  Java_BisonContents_onUpdateUrl(env, java_object_, j_url);
}

void BisonContents::PlatformSetIsLoading(bool loading) {
  JNIEnv* env = AttachCurrentThread();
  Java_BisonContents_setIsLoading(env, java_object_, loading);
}

void BisonContents::PlatformCreateWindow() {
  // java_object_.Reset(CreateShellView(this));
}

void BisonContents::PlatformSetContents() {
  JNIEnv* env = AttachCurrentThread();
  Java_BisonContents_initFromNativeTabContents(
      env, java_object_, web_contents()->GetJavaWebContents());
}

void BisonContents::PlatformResizeSubViews() {
  // Not needed; subviews are bound.
}

void BisonContents::SizeTo(const gfx::Size& content_size) {
  JNIEnv* env = AttachCurrentThread();
  Java_BisonContents_sizeTo(env, java_object_, content_size.width(),
                            content_size.height());
}

void BisonContents::PlatformSetTitle(const base::string16& title) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> jstring_title =
      ConvertUTF8ToJavaString(env, base::UTF16ToUTF8(title));
  Java_BisonContents_onUpdateTitle(env, java_object_, jstring_title);
}

void BisonContents::LoadProgressChanged(WebContents* source, double progress) {
  JNIEnv* env = AttachCurrentThread();
  Java_BisonContents_onLoadProgressChanged(env, java_object_, progress);
}

void BisonContents::SetOverlayMode(bool use_overlay_mode) {
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_BisonContents_setOverlayMode(env, java_object_, use_overlay_mode);
}

void BisonContents::PlatformToggleFullscreenModeForTab(
    WebContents* web_contents,
    bool enter_fullscreen) {
  JNIEnv* env = AttachCurrentThread();
  Java_BisonContents_toggleFullscreenModeForTab(env, java_object_,
                                                enter_fullscreen);
}

bool BisonContents::PlatformIsFullscreenForTabOrPending(
    const WebContents* web_contents) const {
  JNIEnv* env = AttachCurrentThread();
  return Java_BisonContents_isFullscreenForTabOrPending(env, java_object_);
}

void BisonContents::Close() {
  // RemoveShellView(java_object_);
  delete this;
}

ScopedJavaLocalRef<jobject> BisonContents::GetWebContents(JNIEnv* env) {
  return web_contents()->GetJavaWebContents();
}

// static
jlong JNI_BisonContents_Init(JNIEnv* env, const JavaParamRef<jobject>& obj) {
  BisonBrowserContext* browserContext =
      BisonContentBrowserClient::Get()->browser_context();
  BisonContents* bison_contents =
      BisonContents::CreateNewWindow(browserContext, NULL);
  bison_contents->java_object_.Reset(obj);
  return reinterpret_cast<intptr_t>(bison_contents);
}

void JNI_BisonContents_CloseShell(JNIEnv* env, jlong bisonViewPtr) {
  BisonContents* bisonView = reinterpret_cast<BisonContents*>(bisonViewPtr);
  bisonView->Close();
}

}  // namespace bison
