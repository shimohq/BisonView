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
#include "bison/browser/bison_contents_io_thread_client.h"
#include "bison_browser_main_parts.h"
#include "bison_content_browser_client.h"
#include "bison_contents_client_bridge.h"
#include "bison_javascript_dialog_manager.h"
#include "bison_web_contents_delegate.h"
#include "build/build_config.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/picture_in_picture_window_controller.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/resource_context.h"
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
using content::BrowserThread;
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

namespace {

std::string* g_locale_list() {
  static base::NoDestructor<std::string> locale_list;
  return locale_list.get();
}



const void* const kBisonContentsUserDataKey = &kBisonContentsUserDataKey;



class BisonContentsUserData : public base::SupportsUserData::Data {
 public:
  explicit BisonContentsUserData(BisonContents* ptr) : contents_(ptr) {}

  static BisonContents* GetContents(WebContents* web_contents) {
    if (!web_contents)
      return NULL;
    BisonContentsUserData* data = static_cast<BisonContentsUserData*>(
        web_contents->GetUserData(kBisonContentsUserDataKey));
    return data ? data->contents_ : NULL;
  }

 private:
  BisonContents* contents_;
};


}  // namespace

// static
std::string BisonContents::GetLocaleList() {
  return *g_locale_list();
}

BisonContents::BisonContents(std::unique_ptr<WebContents> web_contents)
    : WebContentsObserver(web_contents.get()),
      web_contents_(std::move(web_contents)),
      is_fullscreen_(false),
      headless_(false) {
  web_contents_->SetUserData(bison::kBisonContentsUserDataKey,
                             std::make_unique<BisonContentsUserData>(this));
}

BisonContents::~BisonContents() {
  PlatformCleanUp();
    // if (headless_)
  for (auto it = RenderProcessHost::AllHostsIterator(); !it.IsAtEnd();
        it.Advance()) {
    it.GetCurrentValue()->DisableKeepAliveRefCount();
  }
}

BisonContents* BisonContents::CreateBisonContents(
    std::unique_ptr<WebContents> web_contents) {
  // WebContents* raw_web_contents = web_contents.get();
  BisonContents* bison_view = new BisonContents(std::move(web_contents));
  // bison_view->PlatformCreateWindow();

  // bison_view->PlatformSetContents();

  return bison_view;
}

// void BisonContents::SetBisonContentsCreatedCallback(
//     base::OnceCallback<void(BisonContents*)> bison_view_created_callback) {
//   DCHECK(!bison_view_created_callback_);
//   bison_view_created_callback_ = std::move(bison_view_created_callback);
// }

BisonContents* BisonContents::FromWebContents(WebContents* web_contents) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return BisonContentsUserData::GetContents(web_contents);
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

void BisonContents::UpdateNavigationControls(bool to_different_document) {}

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

void BisonContents::OnDevToolsWebContentsDestroyed() {
  devtools_observer_.reset();
}

void BisonContents::PlatformCleanUp() {
  JNIEnv* env = AttachCurrentThread();
  if (java_object_.is_null())
    return;
  VLOG(0) << "destroy native";
  Java_BisonContents_onNativeDestroyed(env, java_object_);
}

void BisonContents::PlatformCreateWindow() {
  // java_object_.Reset(CreateShellView(this));
}

void BisonContents::SetJavaPeers(
    JNIEnv* env,
    const JavaParamRef<jobject>& web_contents_delegate,
    const JavaParamRef<jobject>& contents_client_bridge,
    const JavaParamRef<jobject>& io_thread_client,
    const JavaParamRef<jobject>& intercept_navigation_delegate) {
  web_contents_delegate_.reset(
      new BisonWebContentsDelegate(env, web_contents_delegate));
  web_contents_->SetDelegate(web_contents_delegate_.get());
  contents_client_bridge_.reset(
      new BisonContentsClientBridge(env, contents_client_bridge));
  BisonContentsClientBridge::Associate(web_contents_.get(),
                                       contents_client_bridge_.get());

  BisonContentsIoThreadClient::Associate(web_contents_.get(), io_thread_client);

  InterceptNavigationDelegate::Associate(
      web_contents_.get(), std::make_unique<InterceptNavigationDelegate>(
                               env, intercept_navigation_delegate));
}

ScopedJavaLocalRef<jobject> BisonContents::GetWebContents(JNIEnv* env) {
  return web_contents()->GetJavaWebContents();
}

void BisonContents::SetExtraHeadersForUrl(
    JNIEnv* env,
    const JavaParamRef<jstring>& url,
    const JavaParamRef<jstring>& jextra_headers) {
  std::string extra_headers;
  if (jextra_headers)
    extra_headers = ConvertJavaStringToUTF8(env, jextra_headers);
  BisonResourceContext* resource_context = static_cast<BisonResourceContext*>(
      BisonBrowserContext::FromWebContents(web_contents_.get())
          ->GetResourceContext());
  resource_context->SetExtraHeaders(GURL(ConvertJavaStringToUTF8(env, url)),
                                    extra_headers);
}

void BisonContents::GrantFileSchemeAccesstoChildProcess(JNIEnv* env) {
  content::ChildProcessSecurityPolicy::GetInstance()->GrantRequestScheme(
      web_contents_->GetMainFrame()->GetProcess()->GetID(), url::kFileScheme);
}

void BisonContents::Destroy(JNIEnv* env) {
  delete this;
}

// static
jlong JNI_BisonContents_Init(JNIEnv* env,
                             const JavaParamRef<jobject>& obj,
                             jlong bison_browser_context) {
  BisonBrowserContext* browserContext =
      reinterpret_cast<BisonBrowserContext*>(bison_browser_context);
  BisonContents* bison_contents =
      BisonContents::CreateNewWindow(browserContext, NULL);
  bison_contents->java_object_.Reset(obj);
  return reinterpret_cast<intptr_t>(bison_contents);
}

}  // namespace bison