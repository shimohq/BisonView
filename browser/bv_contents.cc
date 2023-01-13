#include "bv_contents.h"

#include <stddef.h>

#include <map>
#include <string>
#include <utility>

#include "bison/bison_jni_headers/BvContents_jni.h"
#include "bison/browser/bv_autofill_client.h"
#include "bison/browser/bv_browser_context.h"
#include "bison/browser/bv_browser_main_parts.h"
#include "bison/browser/bv_content_browser_client.h"
#include "bison/browser/bv_contents_client_bridge.h"
#include "bison/browser/bv_contents_io_thread_client.h"
#include "bison/browser/bv_javascript_dialog_manager.h"
#include "bison/browser/bv_pdf_exporter.h"
#include "bison/browser/bv_render_process.h"
#include "bison/browser/bv_renderer_priority.h"
#include "bison/browser/bv_resource_context.h"
#include "bison/browser/bv_settings.h"
#include "bison/browser/bv_web_contents_delegate.h"
#include "bison/browser/js_java_interaction/bv_web_message_host_factory.h"
#include "bison/browser/lifecycle/bv_contents_lifecycle_notifier.h"
#include "bison/browser/permission/bv_permission_request.h"
#include "bison/browser/permission/permission_request_handler.h"
#include "bison/browser/permission/simple_permission_request.h"
#include "bison/browser/state_serializer.h"
#include "bison/common/devtools_instrumentation.h"
#include "bison/common/mojom/frame.mojom.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/locale_utils.h"
#include "base/android/scoped_java_ref.h"
#include "base/atomicops.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/i18n/rtl.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"
#include "base/pickle.h"
#include "base/supports_user_data.h"
#include "base/task/single_thread_task_runner.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/android_autofill/browser/android_autofill_manager.h"
#include "components/android_autofill/browser/autofill_provider_android.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/metrics/content/content_stability_metrics_provider.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "content/public/browser/android/child_process_importance.h"
// #include "content/public/browser/android/synchronous_compositor.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/browsing_data_remover.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_iterator.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/mhtml_generation_params.h"
#include "net/base/auth.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "ui/gfx/android/java_bitmap.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image.h"

using autofill::ContentAutofillDriverFactory;
using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF16;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF16ToJavaString;
using base::android::ConvertUTF8ToJavaString;
using base::android::HasException;
using base::android::JavaParamRef;
using base::android::JavaRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;
using content::RenderFrameHost;
using content::WebContents;
using js_injection::JsCommunicationHost;
using navigation_interception::InterceptNavigationDelegate;

namespace bison {

namespace {

bool g_should_download_favicons = false;

std::string* g_locale() {
  static base::NoDestructor<std::string> locale;
  return locale.get();
}

std::string* g_locale_list() {
  static base::NoDestructor<std::string> locale_list;
  return locale_list.get();
}

const void* const kBvContentsUserDataKey = &kBvContentsUserDataKey;

class BvContentsUserData : public base::SupportsUserData::Data {
 public:
  explicit BvContentsUserData(BvContents* ptr) : contents_(ptr) {}

  static BvContents* GetContents(content::WebContents* web_contents) {
    if (!web_contents)
      return NULL;
    BvContentsUserData* data = static_cast<BvContentsUserData*>(
        web_contents->GetUserData(kBvContentsUserDataKey));
    return data ? data->contents_.get() : NULL;
  }

 private:
  raw_ptr<BvContents> contents_;
};

base::subtle::Atomic32 g_instance_count = 0;

}  // namespace

class ScopedAllowInitGLBindings {
 public:
  ScopedAllowInitGLBindings() {}

  ~ScopedAllowInitGLBindings() {}

 private:
  base::ScopedAllowBlocking allow_blocking_;
};

// static
BvContents* BvContents::FromWebContents(WebContents* web_contents) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return BvContentsUserData::GetContents(web_contents);
}

void JNI_BvContents_UpdateDefaultLocale(
    JNIEnv* env,
    const JavaParamRef<jstring>& locale,
    const JavaParamRef<jstring>& locale_list) {
  *g_locale() = ConvertJavaStringToUTF8(env, locale);
  *g_locale_list() = ConvertJavaStringToUTF8(env, locale_list);
}

// static
std::string BvContents::GetLocale() {
  return *g_locale();
}

// static
std::string BvContents::GetLocaleList() {
  return *g_locale_list();
}

// static
BvBrowserPermissionRequestDelegate* BvBrowserPermissionRequestDelegate::FromID(
    int render_process_id,
    int render_frame_id) {
  BvContents* contents =
      BvContents::FromWebContents(content::WebContents::FromRenderFrameHost(
          content::RenderFrameHost::FromID(render_process_id,
                                           render_frame_id)));
  return contents;
}

// jiang removed

// static
BvRenderProcessGoneDelegate* BvRenderProcessGoneDelegate::FromWebContents(
    content::WebContents* web_contents) {
  return BvContents::FromWebContents(web_contents);
}

BvContents::BvContents(std::unique_ptr<WebContents> web_contents)
    : WebContentsObserver(web_contents.get()),
      web_contents_(std::move(web_contents)) {
  view_visible_ = false;
  window_visible_ = false;
  is_paused_ = false;
  base::subtle::NoBarrier_AtomicIncrement(&g_instance_count, 1);
  icon_helper_ = std::make_unique<IconHelper>(web_contents_.get());
  icon_helper_->SetListener(this);
  web_contents_->SetUserData(bison::kBvContentsUserDataKey,
                             std::make_unique<BvContentsUserData>(this));

  render_view_host_ext_.reset(
      new BvRenderViewHostExt(this, web_contents_.get()));

  permission_request_handler_.reset(
      new PermissionRequestHandler(this, web_contents_.get()));

  BvAutofillClient* autofill_manager_delegate =
      BvAutofillClient::FromWebContents(web_contents_.get());
  if (autofill_manager_delegate)
    InitAutofillIfNecessary(autofill_manager_delegate->GetSaveFormData());
  BvContentsLifecycleNotifier::GetInstance().OnWebViewCreated(this);
}

void BvContents::SetJavaPeers(
    JNIEnv* env,
    const JavaParamRef<jobject>& web_contents_delegate,
    const JavaParamRef<jobject>& contents_client_bridge,
    const JavaParamRef<jobject>& io_thread_client,
    const JavaParamRef<jobject>& intercept_navigation_delegate,
    const JavaParamRef<jobject>& autofill_provider) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  web_contents_delegate_ =
    std::make_unique<BvWebContentsDelegate>(env, web_contents_delegate);
  web_contents_->SetDelegate(web_contents_delegate_.get());

  contents_client_bridge_ =
      std::make_unique<BvContentsClientBridge>(env, contents_client_bridge);
  BvContentsClientBridge::Associate(web_contents_.get(),
                                    contents_client_bridge_.get());

  BvContentsIoThreadClient::Associate(web_contents_.get(), io_thread_client);

  InterceptNavigationDelegate::Associate(
      web_contents_.get(), std::make_unique<InterceptNavigationDelegate>(
                               env, intercept_navigation_delegate));
}

void BvContents::InitializeAndroidAutofill(JNIEnv* env) {
  // Initialize Android Autofill, this method shall only be called in Android O
  // and beyond.
  // AutofillProvider shall already be created for |web_contents_| from
  // AutofillProvider java.
  DCHECK(autofill::AutofillProvider::FromWebContents(web_contents_.get()));
  // Autocomplete is only supported for Android pre-O, disable it if Android
  // autofill is enabled.
  InitAutofillIfNecessary(/*autocomplete_enabled=*/false);
}

void BvContents::SetSaveFormData(bool enabled) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  InitAutofillIfNecessary(enabled);
  // // We need to check for the existence, since autofill_manager_delegate
  // // may not be created when the setting is false.
  if (BvAutofillClient::FromWebContents(web_contents_.get())) {
    BvAutofillClient::FromWebContents(web_contents_.get())
        ->SetSaveFormData(enabled);
  }
}

void BvContents::InitAutofillIfNecessary(bool autocomplete_enabled) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Check if the autofill driver factory already exists.
  content::WebContents* web_contents = web_contents_.get();
  if (ContentAutofillDriverFactory::FromWebContents(web_contents))
    return;

  auto* autofill_provider =
      autofill::AutofillProvider::FromWebContents(web_contents);

  // Just return, if the app neither runs on O sdk nor enables autocomplete.
  if (!autofill_provider && !autocomplete_enabled)
    return;

  autofill::AutofillManager::EnableDownloadManager enable_download_manager(
      !autofill::AutofillProvider::is_download_manager_disabled_for_testing());

  BvAutofillClient::CreateForWebContents(web_contents);

  ContentAutofillDriverFactory::DriverInitCallback driver_init_hook =
      autofill_provider
          ? base::BindRepeating(&autofill::AndroidDriverInitHook,
                                BvAutofillClient::FromWebContents(web_contents),
                                enable_download_manager)
          : base::BindRepeating(&autofill::BrowserDriverInitHook,
                                BvAutofillClient::FromWebContents(web_contents),
                                base::android::GetDefaultLocaleString());

  ContentAutofillDriverFactory::CreateForWebContentsAndDelegate(
      web_contents, BvAutofillClient::FromWebContents(web_contents),
      std::move(driver_init_hook));
}

void BvContents::SetAutofillClient(const JavaRef<jobject>& client) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (!obj)
    return;
  Java_BvContents_setAutofillClient(env, obj, client);
}

BvContents::~BvContents() {
  DCHECK_EQ(this, BvContents::FromWebContents(web_contents_.get()));
  web_contents_->RemoveUserData(kBvContentsUserDataKey);
  BvContentsClientBridge::Dissociate(web_contents_.get());
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  base::subtle::Atomic32 instance_count =
      base::subtle::NoBarrier_AtomicIncrement(&g_instance_count, -1);

  if (instance_count == 0) {
    // TODO(timvolodine): consider moving NotifyMemoryPressure to
    // (crbug.com/522988).
    base::MemoryPressureListener::NotifyMemoryPressure(
        base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL);
  }
  Java_BvContents_onNativeDestroyed(env, obj);
  BvContentsLifecycleNotifier::GetInstance().OnWebViewDestroyed(this);
  WebContentsObserver::Observe(nullptr);
}

ScopedJavaLocalRef<jobject> BvContents::GetWebContents(JNIEnv* env) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(web_contents_);
  if (!web_contents_)
    return base::android::ScopedJavaLocalRef<jobject>();

  return web_contents_->GetJavaWebContents();
}

BvContents* BvContents::CreateBisonContents(content::BrowserContext* browser_context) {
  WebContents::CreateParams create_params(browser_context, nullptr);
  std::unique_ptr<WebContents> web_contents =
      WebContents::Create(create_params);
  // WebContents* raw_web_contents = web_contents.get();
  BvContents* bison_contents = new BvContents(std::move(web_contents));

  return bison_contents;
}

ScopedJavaLocalRef<jobject> BvContents::GetRenderProcess(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::RenderProcessHost* host =
      web_contents_->GetMainFrame()->GetProcess();
  if (host->run_renderer_in_process()) {
    return ScopedJavaLocalRef<jobject>();
  }
  BvRenderProcess* render_process =
      BvRenderProcess::GetInstanceForRenderProcessHost(host);
  return render_process->GetJavaObject();
}

void BvContents::Destroy(JNIEnv* env) {
  java_ref_.reset();
  delete this;
}

jlong JNI_BvContents_Init(JNIEnv* env,
                          const JavaParamRef<jobject>& obj,
                          jlong bv_browser_context) {
  BvBrowserContext* browserContext =
      reinterpret_cast<BvBrowserContext*>(bv_browser_context);
  BvContents* bison_contents = BvContents::CreateBisonContents(browserContext);
  bison_contents->java_ref_ = JavaObjectWeakGlobalRef(env, obj);
  return reinterpret_cast<intptr_t>(bison_contents);
}

// static jboolean JNI_BvContents_HasRequiredHardwareExtensions(JNIEnv* env) {
//   ScopedAllowInitGLBindings scoped_allow_init_gl_bindings;
//   // Make sure GPUInfo is collected. This will initialize GL bindings,
//   // collect GPUInfo, and compute GpuFeatureInfo if they have not been
//   // already done.
//   // jiang
//   // return GpuServiceWebView::GetInstance()
//   //     ->gpu_info()
//   //     .can_support_threaded_texture_mailbox;
//   return true;
// }







// static
jint JNI_BvContents_GetNativeInstanceCount(JNIEnv* env) {
  return base::subtle::NoBarrier_Load(&g_instance_count);
}



namespace {
void DocumentHasImagesCallback(const ScopedJavaGlobalRef<jobject>& message,
                               bool has_images) {
  Java_BvContents_onDocumentHasImagesResponse(AttachCurrentThread(), has_images,
                                              message);
}
}  // namespace

void BvContents::DocumentHasImages(JNIEnv* env,
                                   const JavaParamRef<jobject>& message) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ScopedJavaGlobalRef<jobject> j_message;
  j_message.Reset(env, message);
  render_view_host_ext_->DocumentHasImages(
      base::BindOnce(&DocumentHasImagesCallback, j_message));
}

namespace {
void GenerateMHTMLCallback(const JavaRef<jobject>& callback,
                           const base::FilePath& path,
                           int64_t size) {
  JNIEnv* env = AttachCurrentThread();
  // Android files are UTF8, so the path conversion below is safe.
  Java_BvContents_generateMHTMLCallback(
      env, ConvertUTF8ToJavaString(env, path.AsUTF8Unsafe()), size, callback);
}
}  // namespace

void BvContents::GenerateMHTML(JNIEnv* env,
                               const JavaParamRef<jstring>& jpath,
                               const JavaParamRef<jobject>& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  base::FilePath target_path(ConvertJavaStringToUTF8(env, jpath));
  web_contents_->GenerateMHTML(
      content::MHTMLGenerationParams(target_path),
      base::BindOnce(&GenerateMHTMLCallback,
                     ScopedJavaGlobalRef<jobject>(env, callback), target_path));
}

void BvContents::CreatePdfExporter(JNIEnv* env,
                                   const JavaParamRef<jobject>& pdfExporter) {
  pdf_exporter_.reset(new BvPdfExporter(env, pdfExporter, web_contents_.get()));
}

bool BvContents::OnReceivedHttpAuthRequest(const JavaRef<jobject>& handler,
                                           const std::string& host,
                                           const std::string& realm) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (!obj)
    return false;

  ScopedJavaLocalRef<jstring> jhost = ConvertUTF8ToJavaString(env, host);
  ScopedJavaLocalRef<jstring> jrealm = ConvertUTF8ToJavaString(env, realm);
  devtools_instrumentation::ScopedEmbedderCallbackTask embedder_callback(
      "onReceivedHttpAuthRequest");
  Java_BvContents_onReceivedHttpAuthRequest(env, obj, handler, jhost, jrealm);
  return true;
}

void BvContents::SetOffscreenPreRaster(bool enabled) {
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
    Java_BvContents_onGeolocationPermissionsShowPrompt(env, j_ref, j_origin);
  }
}

void ShowGeolocationPromptHelper(const JavaObjectWeakGlobalRef& java_ref,
                                 const GURL& origin) {
  JNIEnv* env = AttachCurrentThread();
  if (java_ref.get(env).obj()) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(&ShowGeolocationPromptHelperTask, java_ref, origin));
  }
}

}  // namespace

void BvContents::ShowGeolocationPrompt(const GURL& requesting_frame,
                                       PermissionCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  GURL origin = requesting_frame.DeprecatedGetOriginAsURL();
  bool show_prompt = pending_geolocation_prompts_.empty();
  pending_geolocation_prompts_.emplace_back(origin, std::move(callback));
  if (show_prompt) {
    ShowGeolocationPromptHelper(java_ref_, origin);
  }
}

// Invoked from Java
void BvContents::InvokeGeolocationCallback(
    JNIEnv* env,
    jboolean value,
    const JavaParamRef<jstring>& origin) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (pending_geolocation_prompts_.empty())
    return;

  GURL callback_origin(base::android::ConvertJavaStringToUTF16(env, origin));
  if (callback_origin.DeprecatedGetOriginAsURL() ==
      pending_geolocation_prompts_.front().first) {
    std::move(pending_geolocation_prompts_.front().second).Run(value);
    pending_geolocation_prompts_.pop_front();
    if (!pending_geolocation_prompts_.empty()) {
      ShowGeolocationPromptHelper(java_ref_,
                                  pending_geolocation_prompts_.front().first);
    }
  }
}

void BvContents::HideGeolocationPrompt(const GURL& origin) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  bool removed_current_outstanding_callback = false;
  std::list<OriginCallback>::iterator it = pending_geolocation_prompts_.begin();
  while (it != pending_geolocation_prompts_.end()) {
    if ((*it).first == origin.DeprecatedGetOriginAsURL()) {
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
      Java_BvContents_onGeolocationPermissionsHidePrompt(env, j_ref);
    }
    if (!pending_geolocation_prompts_.empty()) {
      ShowGeolocationPromptHelper(java_ref_,
                                  pending_geolocation_prompts_.front().first);
    }
  }
}

void BvContents::OnPermissionRequest(
    base::android::ScopedJavaLocalRef<jobject> j_request,
    BvPermissionRequest* request) {
  DCHECK(j_request);
  DCHECK(request);

  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> j_ref = java_ref_.get(env);
  if (!j_ref) {
    permission_request_handler_->CancelRequest(request->GetOrigin(),
                                               request->GetResources());
    return;
  }

  Java_BvContents_onPermissionRequest(env, j_ref, j_request);
}

void BvContents::OnPermissionRequestCanceled(BvPermissionRequest* request) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> j_request = request->GetJavaObject();
  ScopedJavaLocalRef<jobject> j_ref = java_ref_.get(env);
  if (!j_request || !j_ref)
    return;

  Java_BvContents_onPermissionRequestCanceled(env, j_ref, j_request);
}

void BvContents::PreauthorizePermission(JNIEnv* env,
                                        const JavaParamRef<jobject>& obj,
                                        const JavaParamRef<jstring>& origin,
                                        jlong resources) {
  permission_request_handler_->PreauthorizePermission(
      GURL(base::android::ConvertJavaStringToUTF8(env, origin)), resources);
}

void BvContents::RequestProtectedMediaIdentifierPermission(
    const GURL& origin,
    PermissionCallback callback) {
  permission_request_handler_->SendRequest(
      std::make_unique<SimplePermissionRequest>(
          origin, BvPermissionRequest::ProtectedMediaId, std::move(callback)));
}

void BvContents::CancelProtectedMediaIdentifierPermissionRequests(
    const GURL& origin) {
  permission_request_handler_->CancelRequest(
      origin, BvPermissionRequest::ProtectedMediaId);
}

void BvContents::RequestGeolocationPermission(const GURL& origin,
                                              PermissionCallback callback) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (!obj)
    return;

  if (Java_BvContents_useLegacyGeolocationPermissionAPI(env, obj)) {
    ShowGeolocationPrompt(origin, std::move(callback));
    return;
  }
  permission_request_handler_->SendRequest(
      std::make_unique<SimplePermissionRequest>(
          origin, BvPermissionRequest::Geolocation, std::move(callback)));
}

void BvContents::CancelGeolocationPermissionRequests(const GURL& origin) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (!obj)
    return;

  if (Java_BvContents_useLegacyGeolocationPermissionAPI(env, obj)) {
    HideGeolocationPrompt(origin);
    return;
  }
  permission_request_handler_->CancelRequest(origin,
                                             BvPermissionRequest::Geolocation);
}

void BvContents::RequestMIDISysexPermission(const GURL& origin,
                                            PermissionCallback callback) {
  permission_request_handler_->SendRequest(
      std::make_unique<SimplePermissionRequest>(
          origin, BvPermissionRequest::MIDISysex, std::move(callback)));
}

void BvContents::CancelMIDISysexPermissionRequests(const GURL& origin) {
  permission_request_handler_->CancelRequest(
      origin, BvPermissionRequest::BvPermissionRequest::MIDISysex);
}

void BvContents::FindAllAsync(JNIEnv* env,
                              const JavaParamRef<jstring>& search_string) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  GetFindHelper()->FindAllAsync(ConvertJavaStringToUTF16(env, search_string));
}

void BvContents::FindNext(JNIEnv* env, jboolean forward) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  GetFindHelper()->FindNext(forward);
}

void BvContents::ClearMatches(JNIEnv* env, const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  GetFindHelper()->ClearMatches();
}

void BvContents::ClearCache(JNIEnv* env, jboolean include_disk_files) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BvRenderProcess* bv_render_process =
      BvRenderProcess::GetInstanceForRenderProcessHost(
          web_contents_->GetMainFrame()->GetProcess());

  bv_render_process->ClearCache();

  if (include_disk_files) {
    content::BrowsingDataRemover* remover =
        web_contents_->GetBrowserContext()->GetBrowsingDataRemover();
    remover->Remove(
        base::Time(), base::Time::Max(),
        content::BrowsingDataRemover::DATA_TYPE_CACHE,
        content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB |
            content::BrowsingDataRemover::ORIGIN_TYPE_PROTECTED_WEB);
  }
}

FindHelper* BvContents::GetFindHelper() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!find_helper_.get()) {
    find_helper_ = std::make_unique<FindHelper>(web_contents_.get());
    find_helper_->SetListener(this);
  }
  return find_helper_.get();
}

void BvContents::OnFindResultReceived(int active_ordinal,
                                      int match_count,
                                      bool finished) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;

  Java_BvContents_onFindResultReceived(env, obj, active_ordinal, match_count,
                                       finished);
}

bool BvContents::ShouldDownloadFavicon(const GURL& icon_url) {
  return g_should_download_favicons;
}

void BvContents::OnReceivedIcon(const GURL& icon_url, const SkBitmap& bitmap) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;

  content::NavigationEntry* entry =
      web_contents_->GetController().GetLastCommittedEntry();

  if (entry) {
    entry->GetFavicon().valid = true;
    entry->GetFavicon().url = icon_url;
    entry->GetFavicon().image = gfx::Image::CreateFrom1xBitmap(bitmap);
  }

  ScopedJavaLocalRef<jobject> java_bitmap =
      gfx::ConvertToJavaBitmap(bitmap, gfx::OomBehavior::kReturnNullOnOom);
  if (!java_bitmap) {
    LOG(WARNING) << "Skipping onReceivedIcon; Not enough memory to convert "
                    "icon to Bitmap.";
    return;
  }
  Java_BvContents_onReceivedIcon(env, obj, java_bitmap);
}

void BvContents::OnReceivedTouchIconUrl(const std::string& url,
                                        bool precomposed) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;

  Java_BvContents_onReceivedTouchIconUrl(
      env, obj, ConvertUTF8ToJavaString(env, url), precomposed);
}

bool BvContents::AllowThirdPartyCookies() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BvSettings* settings = BvSettings::FromWebContents(web_contents_.get());
  return settings->GetAllowThirdPartyCookies();
}

void BvContents::OnViewTreeForceDarkStateChanged(
    bool view_tree_force_dark_state) {
  view_tree_force_dark_state_ = view_tree_force_dark_state;
  web_contents_->NotifyPreferencesChanged();
}

base::android::ScopedJavaLocalRef<jbyteArray> BvContents::GetCertificate(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::NavigationEntry* entry =
      web_contents_->GetController().GetLastCommittedEntry();
  if (!entry || !entry->GetSSL().certificate)
    return ScopedJavaLocalRef<jbyteArray>();

  // Convert the certificate and return it
  base::StringPiece der_string = net::x509_util::CryptoBufferAsStringPiece(
      entry->GetSSL().certificate->cert_buffer());
  return base::android::ToJavaByteArray(
      env, reinterpret_cast<const uint8_t*>(der_string.data()),
      der_string.length());
}

void BvContents::RequestNewHitTestDataAt(JNIEnv* env,
                                         const JavaParamRef<jobject>& obj,
                                         jfloat x,
                                         jfloat y,
                                         jfloat touch_major) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  gfx::PointF touch_center(x, y);
  gfx::SizeF touch_area(touch_major, touch_major);
  render_view_host_ext_->RequestNewHitTestDataAt(touch_center, touch_area);
}

void BvContents::UpdateLastHitTestData(JNIEnv* env,
                                       const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!render_view_host_ext_->HasNewHitTestData())
    return;

  const mojom::HitTestData& data =
      render_view_host_ext_->GetLastHitTestData();
  render_view_host_ext_->MarkHitTestDataRead();

  // Make sure to null the Java object if data is empty/invalid.
  ScopedJavaLocalRef<jstring> extra_data_for_type;
  if (data.extra_data_for_type.length())
    extra_data_for_type =
        ConvertUTF8ToJavaString(env, data.extra_data_for_type);

  ScopedJavaLocalRef<jstring> href;
  if (data.href.length())
    href = ConvertUTF16ToJavaString(env, data.href);

  ScopedJavaLocalRef<jstring> anchor_text;
  if (data.anchor_text.length())
    anchor_text = ConvertUTF16ToJavaString(env, data.anchor_text);

  ScopedJavaLocalRef<jstring> img_src;
  if (data.img_src.is_valid())
    img_src = ConvertUTF8ToJavaString(env, data.img_src.spec());

  Java_BvContents_updateHitTestData(env, obj, static_cast<jint>(data.type),
                                    extra_data_for_type, href, anchor_text,
                                    img_src);
}

void BvContents::OnSizeChanged(JNIEnv* env,
                               const JavaParamRef<jobject>& obj,
                               int w,
                               int h,
                               int ow,
                               int oh) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  gfx::Size size(w, h);
  web_contents_->GetNativeView()->OnPhysicalBackingSizeChanged(size);
  web_contents_->GetNativeView()->OnSizeChanged(w, h);
  // browser_view_renderer_.OnSizeChanged(w, h);
}

void BvContents::SetViewVisibility(JNIEnv* env,
                                   const JavaParamRef<jobject>& obj,
                                   bool visible) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  view_visible_ = visible;
}

void BvContents::SetWindowVisibility(JNIEnv* env,
                                     const JavaParamRef<jobject>& obj,
                                     bool visible) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (visible)
    BvContentsLifecycleNotifier::GetInstance().OnWebViewWindowBeVisible(this);
  else
    BvContentsLifecycleNotifier::GetInstance().OnWebViewWindowBeInvisible(this);
  window_visible_ = visible;
}

void BvContents::SetIsPaused(JNIEnv* env,
                             const JavaParamRef<jobject>& obj,
                             bool paused) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  is_paused_ = paused;
}

void BvContents::OnAttachedToWindow(JNIEnv* env,
                                    const JavaParamRef<jobject>& obj,
                                    int w,
                                    int h) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // browser_view_renderer_.OnAttachedToWindow(w, h);
  BvContentsLifecycleNotifier::GetInstance().OnWebViewAttachedToWindow(this);
}

void BvContents::OnDetachedFromWindow(JNIEnv* env,
                                      const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // browser_view_renderer_.OnDetachedFromWindow();
  BvContentsLifecycleNotifier::GetInstance().OnWebViewDetachedFromWindow(this);
}

bool BvContents::IsVisible(JNIEnv* env, const JavaParamRef<jobject>& obj) {
  return is_paused_ ? false : view_visible_ && window_visible_;
}

base::android::ScopedJavaLocalRef<jbyteArray> BvContents::GetOpaqueState(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Required optimization in WebViewClassic to not save any state if
  // there has been no navigations.
  if (!web_contents_->GetController().GetEntryCount())
    return ScopedJavaLocalRef<jbyteArray>();

  base::Pickle pickle;
  WriteToPickle(*web_contents_, &pickle);
  return base::android::ToJavaByteArray(
      env, reinterpret_cast<const uint8_t*>(pickle.data()), pickle.size());
}

jboolean BvContents::RestoreFromOpaqueState(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jbyteArray>& state) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // TODO(boliu): This copy can be optimized out if this is a performance
  // problem.
  std::vector<uint8_t> state_vector;
  base::android::JavaByteArrayToByteVector(env, state, &state_vector);

  base::Pickle pickle(reinterpret_cast<const char*>(state_vector.data()),
                      state_vector.size());
  base::PickleIterator iterator(pickle);

  return RestoreFromPickle(&iterator, web_contents_.get());
}

void BvContents::SetPendingWebContentsForPopup(
    std::unique_ptr<content::WebContents> pending) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (pending_contents_.get()) {
    // TODO(benm): Support holding multiple pop up window requests.
    LOG(WARNING) << "Blocking popup window creation as an outstanding "
                 << "popup window is still pending.";
    base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE,
                                                    pending.release());
    return;
  }
  //jiang
  //pending_contents_ = std::make_unique<BvContents>(std::move(pending));
  // Set dip_scale for pending contents, which is necessary for the later
  // SynchronousCompositor and InputHandler setup.
  //pending_contents_->SetDipScaleInternal(browser_view_renderer_.dip_scale());
}

void BvContents::FocusFirstNode(JNIEnv* env, const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  web_contents_->FocusThroughTabTraversal(false);
}

void BvContents::SetBackgroundColor(JNIEnv* env,
                                    const JavaParamRef<jobject>& obj,
                                    jint color) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  //render_view_host_ext_->SetBackgroundColor(color);
}

void BvContents::ZoomBy(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>& obj,
                        jfloat delta) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // browser_view_renderer_.ZoomBy(delta);
}

void BvContents::SetDipScale(JNIEnv* env,
                             const JavaParamRef<jobject>& obj,
                             jfloat dip_scale) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // SetDipScaleInternal(dip_scale);
}

void BvContents::OnWebLayoutPageScaleFactorChanged(float page_scale_factor) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_BvContents_onWebLayoutPageScaleFactorChanged(env, obj,
                                                    page_scale_factor);
}

void BvContents::OnWebLayoutContentsSizeChanged(
    const gfx::Size& contents_size) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (!obj)
    return;
  // gfx::Size contents_size_css =
  //     content::IsUseZoomForDSFEnabled()
  //         ? ScaleToRoundedSize(contents_size,
  //                              1 / browser_view_renderer_.dip_scale())
  //         : contents_size;
  // Java_BvContents_onWebLayoutContentsSizeChanged(
  //     env, obj, contents_size_css.width(), contents_size_css.height());
}

namespace {
void InvokeVisualStateCallback(const JavaObjectWeakGlobalRef& java_ref,
                               jlong request_id,
                               const JavaRef<jobject>& callback,
                               bool result) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref.get(env);
  if (obj.is_null())
    return;
  Java_BvContents_invokeVisualStateCallback(env, obj, callback, request_id);
}
}  // namespace

void BvContents::InsertVisualStateCallback(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jlong request_id,
    const JavaParamRef<jobject>& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  web_contents_->GetMainFrame()->InsertVisualStateCallback(
      base::BindOnce(&InvokeVisualStateCallback, java_ref_, request_id,
                     ScopedJavaGlobalRef<jobject>(env, callback)));
}

jint BvContents::GetEffectivePriority(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  switch (
      web_contents_->GetMainFrame()->GetProcess()->GetEffectiveImportance()) {
    case content::ChildProcessImportance::NORMAL:
      return static_cast<jint>(RendererPriority::WAIVED);
    case content::ChildProcessImportance::MODERATE:
      return static_cast<jint>(RendererPriority::LOW);
    case content::ChildProcessImportance::IMPORTANT:
      return static_cast<jint>(RendererPriority::HIGH);
  }
  NOTREACHED();
  return 0;
}

JsCommunicationHost* BvContents::GetJsCommunicationHost() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!js_communication_host_.get()) {
    js_communication_host_ =
        std::make_unique<JsCommunicationHost>(web_contents_.get());
  }
  return js_communication_host_.get();
}

jint BvContents::AddDocumentStartJavaScript(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& script,
    const base::android::JavaParamRef<jobjectArray>& allowed_origin_rules) {
  std::vector<std::string> native_allowed_origin_rule_strings;
  AppendJavaStringArrayToStringVector(env, allowed_origin_rules,
                                      &native_allowed_origin_rule_strings);
  auto result = GetJsCommunicationHost()->AddDocumentStartJavaScript(
      base::android::ConvertJavaStringToUTF16(env, script),
      native_allowed_origin_rule_strings);
  if (result.error_message) {
    env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"),
                  result.error_message->data());
    return -1;
  }
  DCHECK(result.script_id);
  return result.script_id.value();
}

void BvContents::RemoveDocumentStartJavaScript(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    jint script_id) {
  GetJsCommunicationHost()->RemoveDocumentStartJavaScript(script_id);
}

base::android::ScopedJavaLocalRef<jstring> BvContents::AddWebMessageListener(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jobject>& listener,
    const base::android::JavaParamRef<jstring>& js_object_name,
    const base::android::JavaParamRef<jobjectArray>& allowed_origin_rules) {
  std::u16string native_js_object_name =
      base::android::ConvertJavaStringToUTF16(env, js_object_name);
  std::vector<std::string> native_allowed_origin_rule_strings;
  AppendJavaStringArrayToStringVector(env, allowed_origin_rules,
                                      &native_allowed_origin_rule_strings);
  const std::u16string error_message =
      GetJsCommunicationHost()->AddWebMessageHostFactory(
          std::make_unique<BvWebMessageHostFactory>(listener),
          native_js_object_name, native_allowed_origin_rule_strings);
  if (error_message.empty())
    return nullptr;
  return base::android::ConvertUTF16ToJavaString(env, error_message);
}

void BvContents::RemoveWebMessageListener(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& js_object_name) {
  GetJsCommunicationHost()->RemoveWebMessageHostFactory(
      ConvertJavaStringToUTF16(env, js_object_name));
}

base::android::ScopedJavaLocalRef<jobjectArray> BvContents::GetJsObjectsInfo(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jclass>& clazz) {
  if (js_communication_host_.get()) {
    return BvWebMessageHostFactory::GetWebMessageListenerInfo(
        GetJsCommunicationHost(), env, clazz);
  }
  return nullptr;
}

void BvContents::SetExtraHeadersForUrl(
    JNIEnv* env,
    const JavaParamRef<jstring>& url,
    const JavaParamRef<jstring>& jextra_headers) {
  std::string extra_headers;
  if (jextra_headers)
    extra_headers = ConvertJavaStringToUTF8(env, jextra_headers);
  BvResourceContext* resource_context = static_cast<BvResourceContext*>(
      BvBrowserContext::FromWebContents(web_contents_.get())
          ->GetResourceContext());
  resource_context->SetExtraHeaders(GURL(ConvertJavaStringToUTF8(env, url)),
                                    extra_headers);
}

void BvContents::SetJsOnlineProperty(JNIEnv* env,
                                     const JavaParamRef<jobject>& obj,
                                     jboolean network_up) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BvRenderProcess* bv_render_process =
      BvRenderProcess::GetInstanceForRenderProcessHost(
          web_contents_->GetMainFrame()->GetProcess());

  bv_render_process->SetJsOnlineProperty(network_up);
}

void BvContents::GrantFileSchemeAccesstoChildProcess(JNIEnv* env) {
  content::ChildProcessSecurityPolicy::GetInstance()->GrantRequestScheme(
      web_contents_->GetMainFrame()->GetProcess()->GetID(), url::kFileScheme);
}

void JNI_BvContents_SetShouldDownloadFavicons(JNIEnv* env) {
  g_should_download_favicons = true;
}






void BvContents::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // If this request was blocked in any way, broadcast an error.
  net::Error error_code = navigation_handle->GetNetErrorCode();
  if (!net::IsRequestBlockedError(error_code) &&
      error_code != net::ERR_ABORTED) {
    return;
  }

  // We do not call OnReceivedError for requests that were blocked due to an
  // interstitial showing. OnReceivedError is handled directly by the blocking
  // page for interstitials.
  if (web_contents_) {
    // We can't be showing an interstitial if there is no web_contents.
    security_interstitials::SecurityInterstitialTabHelper*
        security_interstitial_tab_helper = security_interstitials::
            SecurityInterstitialTabHelper::FromWebContents(web_contents_.get());
    if (security_interstitial_tab_helper &&
        (security_interstitial_tab_helper->IsInterstitialPendingForNavigation(
             navigation_handle->GetNavigationId()) ||
         security_interstitial_tab_helper->IsDisplayingInterstitial())) {
      return;
    }
  }

  BvContentsClientBridge* client =
      BvContentsClientBridge::FromWebContents(web_contents_.get());
  if (!client)
    return;

  BvWebResourceRequest request(navigation_handle->GetURL().spec(),
                               navigation_handle->IsPost() ? "POST" : "GET",
                               navigation_handle->IsInPrimaryMainFrame(),
                               navigation_handle->HasUserGesture(),
                               net::HttpRequestHeaders());
  request.is_renderer_initiated = navigation_handle->IsRendererInitiated();
  client->OnReceivedError(request, error_code, false);
}




void BvContents::RendererUnresponsive(
    content::RenderProcessHost* render_process_host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;

  BvRenderProcess* render_process =
      BvRenderProcess::GetInstanceForRenderProcessHost(render_process_host);
  Java_BvContents_onRendererUnresponsive(env, obj,
                                         render_process->GetJavaObject());
}

void BvContents::RendererResponsive(
    content::RenderProcessHost* render_process_host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;

  BvRenderProcess* render_process =
      BvRenderProcess::GetInstanceForRenderProcessHost(render_process_host);
  Java_BvContents_onRendererResponsive(env, obj,
                                       render_process->GetJavaObject());
}

BvContents::RenderProcessGoneResult BvContents::OnRenderProcessGone(
    int child_process_id,
    bool crashed) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return RenderProcessGoneResult::kHandled;

  bool result =
      Java_BvContents_onRenderProcessGone(env, obj, child_process_id, crashed);

  if (HasException(env))
    return RenderProcessGoneResult::kException;

  return result ? RenderProcessGoneResult::kHandled
                : RenderProcessGoneResult::kUnhandled;
}

// static
ScopedJavaLocalRef<jstring> JNI_BvContents_GetProductVersion(JNIEnv* env) {
  return base::android::ConvertUTF8ToJavaString(
      env, version_info::GetVersionNumber());
}

// static
void JNI_BvContents_LogCommandLineForDebugging(JNIEnv* env) {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  for (const auto& pair : command_line.GetSwitches()) {
    const std::string& key = pair.first;
    const base::CommandLine::StringType& value = pair.second;
    LOG(INFO) << "BisonViewCommandLine '" << key << "': '" << value << "'";
  }
}

}  // namespace bison
