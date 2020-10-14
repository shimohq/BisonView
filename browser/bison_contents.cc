#include "bison_contents.h"

#include <stddef.h>

#include <map>
#include <string>
#include <utility>

#include "bison/bison_jni_headers/BisonContents_jni.h"  // jiang jni.h
#include "bison/browser/bison_browser_main_parts.h"
#include "bison/browser/bison_content_browser_client.h"
#include "bison/browser/bison_contents_client_bridge.h"
#include "bison/browser/bison_contents_io_thread_client.h"
#include "bison/browser/bison_javascript_dialog_manager.h"
#include "bison/browser/bison_settings.h"
#include "bison/browser/bison_web_contents_delegate.h"
#include "bison/browser/permission/bison_permission_request.h"
#include "bison/browser/permission/permission_request_handler.h"
#include "bison/browser/permission/simple_permission_request.h"
#include "bison/common/devtools_instrumentation.h"

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
#include "base/task/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#include "content/public/browser/browser_task_traits.h"
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
using base::android::JavaRef;
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

namespace {

std::string* g_locale() {
  static base::NoDestructor<std::string> locale;
  return locale.get();
}

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

class ScopedAllowInitGLBindings {
 public:
  ScopedAllowInitGLBindings() {}

  ~ScopedAllowInitGLBindings() {}

 private:
  base::ScopedAllowBlocking allow_blocking_;
};

// static
BisonContents* BisonContents::FromWebContents(WebContents* web_contents) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return BisonContentsUserData::GetContents(web_contents);
}

void JNI_BisonContents_UpdateDefaultLocale(
    JNIEnv* env,
    const JavaParamRef<jstring>& locale,
    const JavaParamRef<jstring>& locale_list) {
  *g_locale() = ConvertJavaStringToUTF8(env, locale);
  *g_locale_list() = ConvertJavaStringToUTF8(env, locale_list);
}

// static
std::string BisonContents::GetLocale() {
  return *g_locale();
}

// static
std::string BisonContents::GetLocaleList() {
  return *g_locale_list();
}

// static
BisonBrowserPermissionRequestDelegate*
BisonBrowserPermissionRequestDelegate::FromID(int render_process_id,
                                              int render_frame_id) {
  BisonContents* contents =
      BisonContents::FromWebContents(content::WebContents::FromRenderFrameHost(
          content::RenderFrameHost::FromID(render_process_id,
                                           render_frame_id)));
  return contents;
}

// // static
// AwSafeBrowsingUIManager::UIManagerClient*
// AwSafeBrowsingUIManager::UIManagerClient::FromWebContents(
//     WebContents* web_contents) {
//   return AwContents::FromWebContents(web_contents);
// }

// // static
// AwRenderProcessGoneDelegate* AwRenderProcessGoneDelegate::FromWebContents(
//     content::WebContents* web_contents) {
//   return AwContents::FromWebContents(web_contents);
// }

BisonContents::BisonContents(std::unique_ptr<WebContents> web_contents)
    : WebContentsObserver(web_contents.get()),
      web_contents_(std::move(web_contents)) {
  web_contents_->SetUserData(bison::kBisonContentsUserDataKey,
                             std::make_unique<BisonContentsUserData>(this));

  permission_request_handler_.reset(
      new PermissionRequestHandler(this, web_contents_.get()));
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

void BisonContents::SetSaveFormData(bool enabled) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // InitAutofillIfNecessary(enabled);
  // // We need to check for the existence, since autofill_manager_delegate
  // // may not be created when the setting is false.
  // if (AwAutofillClient::FromWebContents(web_contents_.get())) {
  //   AwAutofillClient::FromWebContents(web_contents_.get())
  //       ->SetSaveFormData(enabled);
  // }
}

BisonContents::~BisonContents() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  VLOG(0) << "destroy native";
  Java_BisonContents_onNativeDestroyed(env, obj);
}

BisonContents* BisonContents::CreateBisonContents(
    BrowserContext* browser_context) {
  WebContents::CreateParams create_params(browser_context, NULL);
  std::unique_ptr<WebContents> web_contents =
      WebContents::Create(create_params);
  // WebContents* raw_web_contents = web_contents.get();
  BisonContents* bison_view = new BisonContents(std::move(web_contents));

  // bison_view->PlatformCreateWindow();



  return bison_view;
}
























void BisonContents::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  net::Error error_code = navigation_handle->GetNetErrorCode();
  if (error_code != net::ERR_BLOCKED_BY_CLIENT &&
      error_code != net::ERR_BLOCKED_BY_ADMINISTRATOR &&
      error_code != net::ERR_ABORTED) {
    return;
  }
  BisonContentsClientBridge* client =
      BisonContentsClientBridge::FromWebContents(web_contents_.get());
  if (!client)
    return;
  BisonWebResourceRequest request(navigation_handle->GetURL().spec(),
                                  navigation_handle->IsPost() ? "POST" : "GET",
                                  navigation_handle->IsInMainFrame(),
                                  navigation_handle->HasUserGesture(),
                                  net::HttpRequestHeaders());
  request.is_renderer_initiated = navigation_handle->IsRendererInitiated();

  client->OnReceivedError(request, error_code);
}





void BisonContents::SetOffscreenPreRaster(bool enabled) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // browser_view_renderer_.SetOffscreenPreRaster(enabled);
}



namespace {












void ShowGeolocationPromptHelperTask(const JavaObjectWeakGlobalRef& java_ref,
                                     const GURL& origin) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> j_ref = java_ref.get(env);
  if (j_ref.obj()) {
    ScopedJavaLocalRef<jstring> j_origin(
        ConvertUTF8ToJavaString(env, origin.spec()));
    devtools_instrumentation::ScopedEmbedderCallbackTask embedder_callback(
        "onGeolocationPermissionsShowPrompt");
    Java_BisonContents_onGeolocationPermissionsShowPrompt(env, j_ref, j_origin);
  }
}

void ShowGeolocationPromptHelper(const JavaObjectWeakGlobalRef& java_ref,
                                 const GURL& origin) {
  JNIEnv* env = AttachCurrentThread();
  if (java_ref.get(env).obj()) {
    base::PostTask(
        FROM_HERE, {content::BrowserThread::UI},
        base::BindOnce(&ShowGeolocationPromptHelperTask, java_ref, origin));
  }
}

}  // namespace

void BisonContents::ShowGeolocationPrompt(
    const GURL& requesting_frame,
    base::OnceCallback<void(bool)> callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  GURL origin = requesting_frame.GetOrigin();
  bool show_prompt = pending_geolocation_prompts_.empty();
  pending_geolocation_prompts_.emplace_back(origin, std::move(callback));
  if (show_prompt) {
    ShowGeolocationPromptHelper(java_ref_, origin);
  }
}

// Invoked from Java
void BisonContents::InvokeGeolocationCallback(
    JNIEnv* env,
    jboolean value,
    const JavaParamRef<jstring>& origin) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (pending_geolocation_prompts_.empty())
    return;

  GURL callback_origin(base::android::ConvertJavaStringToUTF16(env, origin));
  if (callback_origin.GetOrigin() ==
      pending_geolocation_prompts_.front().first) {
    std::move(pending_geolocation_prompts_.front().second).Run(value);
    pending_geolocation_prompts_.pop_front();
    if (!pending_geolocation_prompts_.empty()) {
      ShowGeolocationPromptHelper(java_ref_,
                                  pending_geolocation_prompts_.front().first);
    }
  }
}

void BisonContents::HideGeolocationPrompt(const GURL& origin) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  bool removed_current_outstanding_callback = false;
  std::list<OriginCallback>::iterator it = pending_geolocation_prompts_.begin();
  while (it != pending_geolocation_prompts_.end()) {
    if ((*it).first == origin.GetOrigin()) {
      if (it == pending_geolocation_prompts_.begin()) {
        removed_current_outstanding_callback = true;
      }
      it = pending_geolocation_prompts_.erase(it);
    } else {
      ++it;
    }
  }

  if (removed_current_outstanding_callback) {
    JNIEnv* env = AttachCurrentThread();
    ScopedJavaLocalRef<jobject> j_ref = java_ref_.get(env);
    if (j_ref.obj()) {
      devtools_instrumentation::ScopedEmbedderCallbackTask embedder_callback(
          "onGeolocationPermissionsHidePrompt");
      Java_BisonContents_onGeolocationPermissionsHidePrompt(env, j_ref);
    }
    if (!pending_geolocation_prompts_.empty()) {
      ShowGeolocationPromptHelper(java_ref_,
                                  pending_geolocation_prompts_.front().first);
    }
  }
}

void BisonContents::OnPermissionRequest(
    base::android::ScopedJavaLocalRef<jobject> j_request,
    BisonPermissionRequest* request) {
  DCHECK(!j_request.is_null());
  DCHECK(request);

  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> j_ref = java_ref_.get(env);
  if (j_ref.is_null()) {
    permission_request_handler_->CancelRequest(request->GetOrigin(),
                                               request->GetResources());
    return;
  }

  Java_BisonContents_onPermissionRequest(env, j_ref, j_request);
}

void BisonContents::OnPermissionRequestCanceled(
    BisonPermissionRequest* request) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> j_request = request->GetJavaObject();
  ScopedJavaLocalRef<jobject> j_ref = java_ref_.get(env);
  if (j_request.is_null() || j_ref.is_null())
    return;

  Java_BisonContents_onPermissionRequestCanceled(env, j_ref, j_request);
}



void BisonContents::RequestProtectedMediaIdentifierPermission(
    const GURL& origin,
    base::OnceCallback<void(bool)> callback) {
  permission_request_handler_->SendRequest(
      std::make_unique<SimplePermissionRequest>(
          origin, BisonPermissionRequest::ProtectedMediaId,
          std::move(callback)));
}

void BisonContents::CancelProtectedMediaIdentifierPermissionRequests(
    const GURL& origin) {
  permission_request_handler_->CancelRequest(
      origin, BisonPermissionRequest::ProtectedMediaId);
}

void BisonContents::RequestGeolocationPermission(
    const GURL& origin,
    base::OnceCallback<void(bool)> callback) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;

  if (Java_BisonContents_useLegacyGeolocationPermissionAPI(env, obj)) {
    ShowGeolocationPrompt(origin, std::move(callback));
    return;
  }
  permission_request_handler_->SendRequest(
      std::make_unique<SimplePermissionRequest>(
          origin, BisonPermissionRequest::Geolocation, std::move(callback)));
}

void BisonContents::CancelGeolocationPermissionRequests(const GURL& origin) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;

  if (Java_BisonContents_useLegacyGeolocationPermissionAPI(env, obj)) {
    HideGeolocationPrompt(origin);
    return;
  }
  permission_request_handler_->CancelRequest(
      origin, BisonPermissionRequest::Geolocation);
}

void BisonContents::RequestMIDISysexPermission(
    const GURL& origin,
    base::OnceCallback<void(bool)> callback) {
  permission_request_handler_->SendRequest(
      std::make_unique<SimplePermissionRequest>(
          origin, BisonPermissionRequest::MIDISysex, std::move(callback)));
}

void BisonContents::CancelMIDISysexPermissionRequests(const GURL& origin) {
  permission_request_handler_->CancelRequest(
      origin, BisonPermissionRequest::BisonPermissionRequest::MIDISysex);
}



ScopedJavaLocalRef<jobject> BisonContents::GetWebContents(JNIEnv* env) {
  return web_contents_.get()->GetJavaWebContents();
}





bool BisonContents::AllowThirdPartyCookies() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BisonSettings* settings = BisonSettings::FromWebContents(web_contents_.get());
  return settings->GetAllowThirdPartyCookies();
}

















































void BisonContents::SetDipScale(JNIEnv* env,
                             const JavaParamRef<jobject>& obj,
                             jfloat dip_scale) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  //SetDipScaleInternal(dip_scale);
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



void BisonContents::SetAutofillClient(const JavaRef<jobject>& client) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_BisonContents_setAutofillClient(env, obj, client);
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
      BisonContents::CreateBisonContents(browserContext);
  VLOG(0) << "after create new window";
  bison_contents->java_ref_ = JavaObjectWeakGlobalRef(env, obj);
  return reinterpret_cast<intptr_t>(bison_contents);
}

// static
ScopedJavaLocalRef<jstring> JNI_BisonContents_GetProductVersion(
    JNIEnv* env) {
  return base::android::ConvertUTF8ToJavaString(
      env, version_info::GetVersionNumber());
}

}  // namespace bison