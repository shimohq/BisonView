#include "bison_view.h"

#include <stddef.h>

#include <map>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/no_destructor.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "bison_browser_main_parts.h"
#include "bison_content_browser_client.h"
#include "bison_devtools_frontend.h"
// #include "bison_javascript_dialog_manager.h"
// #include "bison_switches.h"
#include "build/build_config.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
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

std::vector<BisonView*> BisonView::windows_;
base::OnceCallback<void(BisonView*)> BisonView::bison_view_created_callback_;

class BisonView::DevToolsWebContentsObserver : public WebContentsObserver {
 public:
  DevToolsWebContentsObserver(BisonView* bison_view, WebContents* web_contents)
      : WebContentsObserver(web_contents), bison_view_(bison_view) {}

  // WebContentsObserver
  void WebContentsDestroyed() override {
    bison_view_->OnDevToolsWebContentsDestroyed();
  }

 private:
  BisonView* bison_view_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsWebContentsObserver);
};

BisonView::BisonView(std::unique_ptr<WebContents> web_contents,
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

BisonView::~BisonView() {
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

BisonView* BisonView::CreateBisonView(std::unique_ptr<WebContents> web_contents,
                                      bool should_set_delegate) {
  // WebContents* raw_web_contents = web_contents.get();
  BisonView* bison_view =
      new BisonView(std::move(web_contents), should_set_delegate);
  bison_view->PlatformCreateWindow();

  bison_view->PlatformSetContents();

  bison_view->PlatformResizeSubViews();

  return bison_view;
}

void BisonView::CloseAllWindows() {
  DevToolsAgentHost::DetachAllClients();
  std::vector<BisonView*> open_windows(windows_);
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

void BisonView::SetMainMessageLoopQuitClosure(base::OnceClosure quit_closure) {
  *g_quit_main_message_loop = std::move(quit_closure);
}

void BisonView::QuitMainMessageLoopForTesting() {
  DCHECK(*g_quit_main_message_loop);
  std::move(*g_quit_main_message_loop).Run();
}

void BisonView::SetBisonViewCreatedCallback(
    base::OnceCallback<void(BisonView*)> bison_view_created_callback) {
  DCHECK(!bison_view_created_callback_);
  bison_view_created_callback_ = std::move(bison_view_created_callback);
}

BisonView* BisonView::FromWebContents(WebContents* web_contents) {
  for (BisonView* window : windows_) {
    if (window->web_contents() && window->web_contents() == web_contents) {
      return window;
    }
  }
  return nullptr;
}

void BisonView::Initialize() {}

BisonView* BisonView::CreateNewWindow(
    BrowserContext* browser_context,
    const scoped_refptr<SiteInstance>& site_instance) {
  WebContents::CreateParams create_params(browser_context, site_instance);
  std::unique_ptr<WebContents> web_contents =
      WebContents::Create(create_params);
  BisonView* bison_view =
      CreateBisonView(std::move(web_contents), true /* should_set_delegate */);

  return bison_view;
}

BisonView* BisonView::CreateNewWindowWithSessionStorageNamespace(
    BrowserContext* browser_context,
    const GURL& url,
    const scoped_refptr<SiteInstance>& site_instance,
    const gfx::Size& initial_size,
    scoped_refptr<SessionStorageNamespace> session_storage_namespace) {
  WebContents::CreateParams create_params(browser_context, site_instance);
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kForcePresentationReceiverForTesting)) {
    create_params.starting_sandbox_flags =
        blink::kPresentationReceiverSandboxFlags;
  }
  std::map<std::string, scoped_refptr<SessionStorageNamespace>>
      session_storages;
  session_storages[""] = session_storage_namespace;
  std::unique_ptr<WebContents> web_contents =
      WebContents::CreateWithSessionStorage(create_params, session_storages);
  BisonView* bison_view =
      CreateBisonView(std::move(web_contents), true /* should_set_delegate */);
  if (!url.is_empty())
    bison_view->LoadURL(url);
  return bison_view;
}

void BisonView::LoadURL(const GURL& url) {
  LoadURLForFrame(
      url, std::string(),
      ui::PageTransitionFromInt(ui::PAGE_TRANSITION_TYPED |
                                ui::PAGE_TRANSITION_FROM_ADDRESS_BAR));
}

void BisonView::LoadURLForFrame(const GURL& url,
                                const std::string& frame_name,
                                ui::PageTransition transition_type) {
  NavigationController::LoadURLParams params(url);
  params.frame_name = frame_name;
  params.transition_type = transition_type;
  web_contents_->GetController().LoadURLWithParams(params);
  web_contents_->Focus();
}

void BisonView::LoadDataWithBaseURL(const GURL& url,
                                    const std::string& data,
                                    const GURL& base_url) {
  bool load_as_string = false;
  LoadDataWithBaseURLInternal(url, data, base_url, load_as_string);
}

#if defined(OS_ANDROID)
void BisonView::LoadDataAsStringWithBaseURL(const GURL& url,
                                            const std::string& data,
                                            const GURL& base_url) {
  bool load_as_string = true;
  LoadDataWithBaseURLInternal(url, data, base_url, load_as_string);
}
#endif

void BisonView::LoadDataWithBaseURLInternal(const GURL& url,
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

void BisonView::AddNewContents(WebContents* source,
                               std::unique_ptr<WebContents> new_contents,
                               WindowOpenDisposition disposition,
                               const gfx::Rect& initial_rect,
                               bool user_gesture,
                               bool* was_blocked) {
  VLOG(0) << "AddNewContents";
}

void BisonView::GoBackOrForward(int offset) {
  web_contents_->GetController().GoToOffset(offset);
  web_contents_->Focus();
}

void BisonView::Reload() {
  web_contents_->GetController().Reload(ReloadType::NORMAL, false);
  web_contents_->Focus();
}

void BisonView::ReloadBypassingCache() {
  web_contents_->GetController().Reload(ReloadType::BYPASSING_CACHE, false);
  web_contents_->Focus();
}

void BisonView::Stop() {
  web_contents_->Stop();
  web_contents_->Focus();
}

void BisonView::UpdateNavigationControls(bool to_different_document) {
  int current_index = web_contents_->GetController().GetCurrentEntryIndex();
  int max_index = web_contents_->GetController().GetEntryCount() - 1;

  PlatformEnableUIControl(BACK_BUTTON, current_index > 0);
  PlatformEnableUIControl(FORWARD_BUTTON, current_index < max_index);
  PlatformEnableUIControl(STOP_BUTTON,
                          to_different_document && web_contents_->IsLoading());
}

void BisonView::ShowDevTools() {
  if (!devtools_frontend_) {
    devtools_frontend_ = BisonDevToolsFrontend::Show(web_contents());
    devtools_observer_.reset(new DevToolsWebContentsObserver(
        this, devtools_frontend_->frontend_shell()->web_contents()));
  }

  devtools_frontend_->Activate();
  devtools_frontend_->Focus();
}

void BisonView::CloseDevTools() {
  if (!devtools_frontend_)
    return;
  devtools_observer_.reset();
  devtools_frontend_->Close();
  devtools_frontend_ = nullptr;
}

gfx::NativeView BisonView::GetContentView() {
  if (!web_contents_)
    return nullptr;
  return web_contents_->GetNativeView();
}

WebContents* BisonView::OpenURLFromTab(WebContents* source,
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

void BisonView::LoadingStateChanged(WebContents* source,
                                    bool to_different_document) {
  UpdateNavigationControls(to_different_document);
  PlatformSetIsLoading(source->IsLoading());
}

void BisonView::EnterFullscreenModeForTab(
    WebContents* web_contents,
    const GURL& origin,
    const blink::mojom::FullscreenOptions& options) {
  ToggleFullscreenModeForTab(web_contents, true);
}

void BisonView::ExitFullscreenModeForTab(WebContents* web_contents) {
  ToggleFullscreenModeForTab(web_contents, false);
}

void BisonView::ToggleFullscreenModeForTab(WebContents* web_contents,
                                           bool enter_fullscreen) {
#if defined(OS_ANDROID)
  PlatformToggleFullscreenModeForTab(web_contents, enter_fullscreen);
#endif
  if (is_fullscreen_ != enter_fullscreen) {
    is_fullscreen_ = enter_fullscreen;
    web_contents->GetRenderViewHost()
        ->GetWidget()
        ->SynchronizeVisualProperties();
  }
}

bool BisonView::IsFullscreenForTabOrPending(const WebContents* web_contents) {
#if defined(OS_ANDROID)
  return PlatformIsFullscreenForTabOrPending(web_contents);
#else
  return is_fullscreen_;
#endif
}

blink::mojom::DisplayMode BisonView::GetDisplayMode(
    const WebContents* web_contents) {
  // TODO: should return blink::mojom::DisplayModeFullscreen wherever user puts
  // a browser window into fullscreen (not only in case of renderer-initiated
  // fullscreen mode): crbug.com/476874.
  return IsFullscreenForTabOrPending(web_contents)
             ? blink::mojom::DisplayMode::kFullscreen
             : blink::mojom::DisplayMode::kBrowser;
}

void BisonView::RequestToLockMouse(WebContents* web_contents,
                                   bool user_gesture,
                                   bool last_unlocked_by_target) {
  web_contents->GotResponseToLockMouseRequest(true);
}

void BisonView::CloseContents(WebContents* source) {
  Close();
}

bool BisonView::CanOverscrollContent() {
#if defined(USE_AURA)
  return true;
#else
  return false;
#endif
}

void BisonView::DidNavigateMainFramePostCommit(WebContents* web_contents) {
  PlatformSetAddressBarURL(web_contents->GetVisibleURL());
}

// JavaScriptDialogManager* BisonView::GetJavaScriptDialogManager(
//     WebContents* source) {
//   if (!dialog_manager_) {
//     dialog_manager_.reset(new ShellJavaScriptDialogManager);
//   }
//   return dialog_manager_.get();
// }

std::unique_ptr<BluetoothChooser> BisonView::RunBluetoothChooser(
    RenderFrameHost* frame,
    const BluetoothChooser::EventHandler& event_handler) {
  // BlinkTestController* blink_test_controller = BlinkTestController::Get();
  // if (blink_test_controller && switches::IsRunWebTestsSwitchPresent())
  //   return blink_test_controller->RunBluetoothChooser(frame, event_handler);
  return nullptr;
}

std::unique_ptr<BluetoothScanningPrompt> BisonView::ShowBluetoothScanningPrompt(
    RenderFrameHost* frame,
    const BluetoothScanningPrompt::EventHandler& event_handler) {
  // return std::make_unique<FakeBluetoothScanningPrompt>(event_handler);
  return nullptr;
}

bool BisonView::DidAddMessageToConsole(
    WebContents* source,
    blink::mojom::ConsoleMessageLevel log_level,
    const base::string16& message,
    int32_t line_no,
    const base::string16& source_id) {
  return false;
}

void BisonView::PortalWebContentsCreated(WebContents* portal_web_contents) {}

void BisonView::RendererUnresponsive(
    WebContents* source,
    RenderWidgetHost* render_widget_host,
    base::RepeatingClosure hang_monitor_restarter) {
  // BlinkTestController* blink_test_controller = BlinkTestController::Get();
  // if (blink_test_controller && switches::IsRunWebTestsSwitchPresent())
  //   blink_test_controller->RendererUnresponsive();
}

void BisonView::ActivateContents(WebContents* contents) {
  contents->GetRenderViewHost()->GetWidget()->Focus();
}

std::unique_ptr<WebContents> BisonView::SwapWebContents(
    WebContents* old_contents,
    std::unique_ptr<WebContents> new_contents,
    bool did_start_load,
    bool did_finish_load) {
  DCHECK_EQ(old_contents, web_contents_.get());
  new_contents->SetDelegate(this);
  web_contents_->SetDelegate(nullptr);
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

bool BisonView::ShouldAllowRunningInsecureContent(WebContents* web_contents,
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

PictureInPictureResult BisonView::EnterPictureInPicture(
    WebContents* web_contents,
    const viz::SurfaceId& surface_id,
    const gfx::Size& natural_size) {
  // During tests, returning success to pretend the window was created and allow
  // tests to run accordingly.
  return PictureInPictureResult::kSuccess;
}

bool BisonView::ShouldResumeRequestsForCreatedWindow() {
  return !delay_popup_contents_delegate_for_testing_;
}

void BisonView::TitleWasSet(NavigationEntry* entry) {
  if (entry)
    PlatformSetTitle(entry->GetTitle());
}

void BisonView::OnDevToolsWebContentsDestroyed() {
  devtools_observer_.reset();
  devtools_frontend_ = nullptr;
}

}  // namespace bison
