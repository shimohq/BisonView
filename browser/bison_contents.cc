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
#include "bison/bison_jni_headers/BisonContents_jni.h"  // jiang jni.h
#include "bison_browser_main_parts.h"
#include "bison_content_browser_client.h"
#include "bison_contents_client_bridge.h"
#include "bison_devtools_frontend.h"
#include "bison_javascript_dialog_manager.h"
#include "bison_web_contents_delegate.h"
#include "build/build_config.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
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
using navigation_interception::InterceptNavigationDelegate;

namespace bison {

// Null until/unless the default main message loop is running.
base::NoDestructor<base::OnceClosure> g_quit_main_message_loop;

std::vector<BisonContents*> BisonContents::windows_;
// base::OnceCallback<void(BisonContents*)>
//     BisonContents::bison_view_created_callback_;

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

BisonContents::BisonContents(std::unique_ptr<WebContents> web_contents)
    : WebContentsObserver(web_contents.get()),
      web_contents_(std::move(web_contents)),
      devtools_frontend_(nullptr),
      is_fullscreen_(false),
      window_(nullptr),
      headless_(false) {
  windows_.push_back(this);

  // if (bison_view_created_callback_)
  //   std::move(bison_view_created_callback_).Run(this);
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
    std::unique_ptr<WebContents> web_contents) {
  // WebContents* raw_web_contents = web_contents.get();
  BisonContents* bison_view = new BisonContents(std::move(web_contents));
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

// void BisonContents::SetBisonContentsCreatedCallback(
//     base::OnceCallback<void(BisonContents*)> bison_view_created_callback) {
//   DCHECK(!bison_view_created_callback_);
//   bison_view_created_callback_ = std::move(bison_view_created_callback);
// }

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
  BisonContents* bison_view = CreateBisonContents(std::move(web_contents));

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

void BisonContents::ToggleFullscreenModeForTab(WebContents* web_contents,
                                               bool enter_fullscreen) {
  // PlatformToggleFullscreenModeForTab(web_contents, enter_fullscreen);
  if (is_fullscreen_ != enter_fullscreen) {
    is_fullscreen_ = enter_fullscreen;
    web_contents->GetRenderViewHost()
        ->GetWidget()
        ->SynchronizeVisualProperties();
  }
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
  if (entry) {
    JNIEnv* env = AttachCurrentThread();
    ScopedJavaLocalRef<jstring> jstring_title =
        ConvertUTF8ToJavaString(env, base::UTF16ToUTF8(entry->GetTitle()));
    Java_BisonContents_onUpdateTitle(env, java_object_, jstring_title);
  }
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

void BisonContents::Close() {
  // RemoveShellView(java_object_);
  delete this;
}

void BisonContents::SetJavaPeers(
    JNIEnv* env,
    const JavaParamRef<jobject>& web_contents_delegate,
    const JavaParamRef<jobject>& contents_client_bridge) {
  web_contents_delegate_.reset(
      new BisonWebContentsDelegate(env, web_contents_delegate));
  web_contents_->SetDelegate(web_contents_delegate_.get());
  contents_client_bridge_.reset(
      new BisonContentsClientBridge(env, contents_client_bridge));
  BisonContentsClientBridge::Associate(web_contents_.get(),
                                       contents_client_bridge_.get());

  // InterceptNavigationDelegate::Associate(
  //     web_contents_.get(), std::make_unique<InterceptNavigationDelegate>(
  //                              env, intercept_navigation_delegate));
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