// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/core/browser/bison_content_browser_client.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "bison/core/browser/bison_browser_context.h"
#include "bison/core/browser/bison_browser_main_parts.h"
#include "bison/core/browser/bison_content_browser_overlay_manifest.h"
#include "bison/core/browser/bison_content_renderer_overlay_manifest.h"
#include "bison/core/browser/bison_contents.h"
#include "bison/core/browser/bison_contents_client_bridge.h"
#include "bison/core/browser/bison_contents_io_thread_client.h"
#include "bison/core/browser/bison_cookie_access_policy.h"
#include "bison/core/browser/bison_devtools_manager_delegate.h"
#include "bison/core/browser/bison_feature_list_creator.h"
#include "bison/core/browser/bison_http_auth_handler.h"
#include "bison/core/browser/bison_quota_permission_context.h"
#include "bison/core/browser/bison_resource_context.h"
#include "bison/core/browser/bison_settings.h"
#include "bison/core/browser/bison_speech_recognition_manager_delegate.h"
#include "bison/core/browser/bison_web_contents_view_delegate.h"
#include "bison/core/browser/cookie_manager.h"
#include "bison/core/browser/network_service/bison_proxy_config_monitor.h"
#include "bison/core/browser/network_service/bison_proxying_restricted_cookie_manager.h"
#include "bison/core/browser/network_service/bison_proxying_url_loader_factory.h"
#include "bison/core/browser/network_service/bison_url_loader_throttle.h"
#include "bison/core/browser/safe_browsing/bison_url_checker_delegate_impl.h"
#include "bison/core/browser/tracing/bison_tracing_delegate.h"
#include "bison/core/common/bison_content_client.h"
#include "bison/core/common/bison_descriptors.h"
#include "bison/core/common/bison_features.h"
#include "bison/core/common/bison_switches.h"
#include "bison/core/common/render_view_messages.h"
#include "bison/core/common/url_constants.h"
#include "base/android/locale_utils.h"
#include "base/base_paths_android.h"
#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/files/scoped_file.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/cdm/browser/cdm_message_filter_android.h"
#include "components/cdm/browser/media_drm_storage_impl.h"
#include "components/content_capture/browser/content_capture_receiver_manager.h"
#include "components/crash/content/browser/crash_handler_host_linux.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#include "components/page_load_metrics/browser/metrics_navigation_throttle.h"
#include "components/page_load_metrics/browser/metrics_web_contents_observer.h"
#include "components/policy/content/policy_blacklist_navigation_throttle.h"
#include "components/policy/core/browser/browser_policy_connector_base.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/browser/browser_url_loader_throttle.h"
#include "components/safe_browsing/browser/mojo_safe_browsing_impl.h"
#include "components/safe_browsing/features.h"
#include "components/spellcheck/spellcheck_buildflags.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "content/public/browser/file_url_loader.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/shared_cors_origin_access_list.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_descriptors.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/user_agent.h"
#include "content/public/common/web_preferences.h"
#include "media/mojo/buildflags.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/android/network_library.h"
#include "net/http/http_util.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "net/ssl/ssl_info.h"
#include "services/network/network_service.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/cookie_manager.mojom-forward.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "storage/browser/quota/quota_settings.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/resource/resource_bundle_android.h"
#include "ui/display/display.h"
#include "ui/resources/grit/ui_resources.h"

#if BUILDFLAG(ENABLE_SPELLCHECK)
#include "components/spellcheck/browser/spell_check_host_impl.h"
#endif

using content::BrowserThread;
using content::ResourceType;
using content::WebContents;

namespace bison {
namespace {
static bool g_should_create_thread_pool = true;
#if DCHECK_IS_ON()
// A boolean value to determine if the NetworkContext has been created yet. This
// exists only to check correctness: g_check_cleartext_permitted may only be set
// before the NetworkContext has been created (otherwise,
// g_check_cleartext_permitted won't have any effect).
bool g_created_network_context_params = false;
#endif

// On apps targeting API level O or later, check cleartext is enforced.
bool g_check_cleartext_permitted = false;

// TODO(sgurun) move this to its own file.
// This class filters out incoming aw_contents related IPC messages for the
// renderer process on the IPC thread.
class BisonContentsMessageFilter : public content::BrowserMessageFilter {
 public:
  explicit BisonContentsMessageFilter(int process_id);

  // BrowserMessageFilter methods.
  void OverrideThreadForMessage(const IPC::Message& message,
                                BrowserThread::ID* thread) override;
  bool OnMessageReceived(const IPC::Message& message) override;

  void OnShouldOverrideUrlLoading(int routing_id,
                                  const base::string16& url,
                                  bool has_user_gesture,
                                  bool is_redirect,
                                  bool is_main_frame,
                                  bool* ignore_navigation);
  void OnSubFrameCreated(int parent_render_frame_id, int child_render_frame_id);

 private:
  ~BisonContentsMessageFilter() override;

  int process_id_;

  DISALLOW_COPY_AND_ASSIGN(BisonContentsMessageFilter);
};

BisonContentsMessageFilter::BisonContentsMessageFilter(int process_id)
    : BrowserMessageFilter(AndroidWebViewMsgStart), process_id_(process_id) {}

BisonContentsMessageFilter::~BisonContentsMessageFilter() = default;

void BisonContentsMessageFilter::OverrideThreadForMessage(
    const IPC::Message& message,
    BrowserThread::ID* thread) {
  if (message.type() == AwViewHostMsg_ShouldOverrideUrlLoading::ID) {
    *thread = BrowserThread::UI;
  }
}

bool BisonContentsMessageFilter::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(BisonContentsMessageFilter, message)
    IPC_MESSAGE_HANDLER(AwViewHostMsg_ShouldOverrideUrlLoading,
                        OnShouldOverrideUrlLoading)
    IPC_MESSAGE_HANDLER(AwViewHostMsg_SubFrameCreated, OnSubFrameCreated)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void BisonContentsMessageFilter::OnShouldOverrideUrlLoading(
    int render_frame_id,
    const base::string16& url,
    bool has_user_gesture,
    bool is_redirect,
    bool is_main_frame,
    bool* ignore_navigation) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  *ignore_navigation = false;
  BisonContentsClientBridge* client =
      BisonContentsClientBridge::FromID(process_id_, render_frame_id);
  if (client) {
    if (!client->ShouldOverrideUrlLoading(url, has_user_gesture, is_redirect,
                                          is_main_frame, ignore_navigation)) {
      // If the shouldOverrideUrlLoading call caused a java exception we should
      // always return immediately here!
      return;
    }
  } else {
    LOG(WARNING) << "Failed to find the associated render view host for url: "
                 << url;
  }
}

void BisonContentsMessageFilter::OnSubFrameCreated(int parent_render_frame_id,
                                                int child_render_frame_id) {
  BisonContentsIoThreadClient::SubFrameCreated(process_id_, parent_render_frame_id,
                                            child_render_frame_id);
}

// A dummy binder for mojo interface autofill::mojom::PasswordManagerDriver.
void DummyBindPasswordManagerDriver(
    mojo::PendingReceiver<autofill::mojom::PasswordManagerDriver> receiver,
    content::RenderFrameHost* render_frame_host) {}

void PassMojoCookieManagerToBisonCookieManager(
    CookieManager* cookie_manager,
    const mojo::Remote<network::mojom::NetworkContext>& network_context) {
  // Get the CookieManager from the NetworkContext.
  mojo::PendingRemote<network::mojom::CookieManager> cookie_manager_remote;
  network_context->GetCookieManager(
      cookie_manager_remote.InitWithNewPipeAndPassReceiver());

  // Pass the mojo::PendingRemote<network::mojom::CookieManager> to
  // bison::CookieManager, so it can implement its APIs with this mojo
  // CookieManager.
  cookie_manager->SetMojoCookieManager(std::move(cookie_manager_remote));
}

#if BUILDFLAG(ENABLE_MOJO_CDM)
void CreateOriginId(cdm::MediaDrmStorageImpl::OriginIdObtainedCB callback) {
  std::move(callback).Run(true, base::UnguessableToken::Create());
}

void AllowEmptyOriginIdCB(base::OnceCallback<void(bool)> callback) {
  // Since CreateOriginId() always returns a non-empty origin ID, we don't need
  // to allow empty origin ID.
  std::move(callback).Run(false);
}

void CreateMediaDrmStorage(content::RenderFrameHost* render_frame_host,
                           ::media::mojom::MediaDrmStorageRequest request) {
  DCHECK(render_frame_host);

  if (render_frame_host->GetLastCommittedOrigin().opaque()) {
    DVLOG(1) << __func__ << ": Unique origin.";
    return;
  }

  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  DCHECK(web_contents) << "WebContents not available.";

  auto* aw_browser_context =
      static_cast<BisonBrowserContext*>(web_contents->GetBrowserContext());
  DCHECK(aw_browser_context) << "BisonBrowserContext not available.";

  PrefService* pref_service = aw_browser_context->GetPrefService();
  DCHECK(pref_service);

  // The object will be deleted on connection error, or when the frame navigates
  // away.
  new cdm::MediaDrmStorageImpl(
      render_frame_host, pref_service, base::BindRepeating(&CreateOriginId),
      base::BindRepeating(&AllowEmptyOriginIdCB), std::move(request));
}
#endif  // BUILDFLAG(ENABLE_MOJO_CDM)

}  // anonymous namespace

std::string GetProduct() {
  return version_info::GetProductNameAndVersionForUserAgent();
}

std::string GetUserAgent() {
  // "Version/4.0" had been hardcoded in the legacy WebView.
  std::string product = "Version/4.0 " + GetProduct();
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kUseMobileUserAgent)) {
    product += " Mobile";
  }
  return content::BuildUserAgentFromProductAndExtraOSInfo(
      product, "; wv", true /* include_android_build_number */);
}

// TODO(yirui): can use similar logic as in PrependToAcceptLanguagesIfNecessary
// in chrome/browser/android/preferences/pref_service_bridge.cc
// static
std::string BisonContentBrowserClient::GetAcceptLangsImpl() {
  // Start with the current locale(s) in BCP47 format.
  std::string locales_string = BisonContents::GetLocaleList();

  // If accept languages do not contain en-US, add in en-US which will be
  // used with a lower q-value.
  if (locales_string.find("en-US") == std::string::npos)
    locales_string += ",en-US";
  return locales_string;
}

// static
void BisonContentBrowserClient::set_check_cleartext_permitted(bool permitted) {
#if DCHECK_IS_ON()
  DCHECK(!g_created_network_context_params);
#endif
  g_check_cleartext_permitted = permitted;
}

// static
bool BisonContentBrowserClient::get_check_cleartext_permitted() {
  return g_check_cleartext_permitted;
}

BisonContentBrowserClient::BisonContentBrowserClient(
    BisonFeatureListCreator* aw_feature_list_creator)
    : aw_feature_list_creator_(aw_feature_list_creator) {
  // |aw_feature_list_creator| should not be null. The BisonBrowserContext will
  // take the PrefService owned by the creator as the Local State instead
  // of loading the JSON file from disk.
  DCHECK(aw_feature_list_creator_);

  // Although WebView does not support password manager feature, renderer code
  // could still request this interface, so we register a dummy binder which
  // just drops the incoming request, to avoid the 'Failed to locate a binder
  // for interface' error log..
  frame_interfaces_.AddInterface(
      base::BindRepeating(&DummyBindPasswordManagerDriver));
  sniff_file_urls_ = BisonSettings::GetAllowSniffingFileUrls();
}

BisonContentBrowserClient::~BisonContentBrowserClient() {}

void BisonContentBrowserClient::OnNetworkServiceCreated(
    network::mojom::NetworkService* network_service) {
  network::mojom::HttpAuthStaticParamsPtr auth_static_params =
      network::mojom::HttpAuthStaticParams::New();
  auth_static_params->supported_schemes = BisonBrowserContext::GetAuthSchemes();
  content::GetNetworkService()->SetUpHttpAuth(std::move(auth_static_params));
}

mojo::Remote<network::mojom::NetworkContext>
BisonContentBrowserClient::CreateNetworkContext(
    content::BrowserContext* context,
    bool in_memory,
    const base::FilePath& relative_partition_path) {
  DCHECK(context);

  content::GetNetworkService()->ConfigureHttpAuthPrefs(
      BisonBrowserProcess::GetInstance()->CreateHttpAuthDynamicParams());

  auto* aw_context = static_cast<BisonBrowserContext*>(context);
  mojo::Remote<network::mojom::NetworkContext> network_context;
  network::mojom::NetworkContextParamsPtr context_params =
      aw_context->GetNetworkContextParams(in_memory, relative_partition_path);
#if DCHECK_IS_ON()
  g_created_network_context_params = true;
#endif
  content::GetNetworkService()->CreateNetworkContext(
      network_context.BindNewPipeAndPassReceiver(), std::move(context_params));

  // Pass a CookieManager to the code supporting BisonCookieManager.java (i.e., the
  // Cookies APIs).
  PassMojoCookieManagerToBisonCookieManager(aw_context->GetCookieManager(),
                                         network_context);

  return network_context;
}

BisonBrowserContext* BisonContentBrowserClient::InitBrowserContext() {
  browser_context_ = std::make_unique<BisonBrowserContext>();
  return browser_context_.get();
}

std::unique_ptr<content::BrowserMainParts>
BisonContentBrowserClient::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters) {
  return std::make_unique<BisonBrowserMainParts>(this);
}

content::WebContentsViewDelegate*
BisonContentBrowserClient::GetWebContentsViewDelegate(
    content::WebContents* web_contents) {
  return BisonWebContentsViewDelegate::Create(web_contents);
}

void BisonContentBrowserClient::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  // Grant content: scheme access to the whole renderer process, since we impose
  // per-view access checks, and access is granted by default (see
  // BisonSettings.mAllowContentUrlAccess).
  content::ChildProcessSecurityPolicy::GetInstance()->GrantRequestScheme(
      host->GetID(), url::kContentScheme);

  host->AddFilter(new BisonContentsMessageFilter(host->GetID()));
  // WebView always allows persisting data.
  host->AddFilter(new cdm::CdmMessageFilterAndroid(true, false));
}

bool BisonContentBrowserClient::IsExplicitNavigation(
    ui::PageTransition transition) {
  return ui::PageTransitionCoreTypeIs(transition, ui::PAGE_TRANSITION_TYPED);
}

bool BisonContentBrowserClient::ShouldUseMobileFlingCurve() {
  return true;
}

bool BisonContentBrowserClient::IsHandledURL(const GURL& url) {
  if (!url.is_valid()) {
    // We handle error cases.
    return true;
  }

  const std::string scheme = url.scheme();
  DCHECK_EQ(scheme, base::ToLowerASCII(scheme));
  // See CreateJobFactory in aw_url_request_context_getter.cc for the
  // list of protocols that are handled.
  // TODO(mnaganov): Make this automatic.
  static const char* const kProtocolList[] = {
    url::kDataScheme,
    url::kBlobScheme,
    url::kFileSystemScheme,
    content::kChromeUIScheme,
    url::kContentScheme,
  };
  if (scheme == url::kFileScheme) {
    // Return false for the "special" file URLs, so they can be loaded
    // even if access to file: scheme is not granted to the child process.
    return !IsAndroidSpecialFileUrl(url);
  }
  for (size_t i = 0; i < base::size(kProtocolList); ++i) {
    if (scheme == kProtocolList[i])
      return true;
  }
  return net::URLRequest::IsHandledProtocol(scheme);
}

bool BisonContentBrowserClient::ForceSniffingFileUrlsForHtml() {
  return sniff_file_urls_;
}

void BisonContentBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int child_process_id) {
  if (!command_line->HasSwitch(switches::kSingleProcess)) {
    // The only kind of a child process WebView can have is renderer or utility.
    std::string process_type =
        command_line->GetSwitchValueASCII(switches::kProcessType);
    DCHECK(process_type == switches::kRendererProcess ||
           process_type == switches::kUtilityProcess)
        << process_type;
    // Pass crash reporter enabled state to renderer processes.
    if (base::CommandLine::ForCurrentProcess()->HasSwitch(
            ::switches::kEnableCrashReporter)) {
      command_line->AppendSwitch(::switches::kEnableCrashReporter);
    }
    if (base::CommandLine::ForCurrentProcess()->HasSwitch(
            ::switches::kEnableCrashReporterForTesting)) {
      command_line->AppendSwitch(::switches::kEnableCrashReporterForTesting);
    }
  }
}

std::string BisonContentBrowserClient::GetApplicationLocale() {
  return base::android::GetDefaultLocaleString();
}

std::string BisonContentBrowserClient::GetAcceptLangs(
    content::BrowserContext* context) {
  return GetAcceptLangsImpl();
}

gfx::ImageSkia BisonContentBrowserClient::GetDefaultFavicon() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  // TODO(boliu): Bundle our own default favicon?
  return rb.GetImageNamed(IDR_DEFAULT_FAVICON).AsImageSkia();
}

bool BisonContentBrowserClient::AllowAppCache(const GURL& manifest_url,
                                           const GURL& first_party,
                                           content::BrowserContext* context) {
  // WebView doesn't have a per-site policy for locally stored data,
  // instead AppCache can be disabled for individual WebViews.
  return true;
}

scoped_refptr<content::QuotaPermissionContext>
BisonContentBrowserClient::CreateQuotaPermissionContext() {
  return new BisonQuotaPermissionContext;
}

void BisonContentBrowserClient::GetQuotaSettings(
    content::BrowserContext* context,
    content::StoragePartition* partition,
    storage::OptionalQuotaSettingsCallback callback) {
  storage::GetNominalDynamicSettings(
      partition->GetPath(), context->IsOffTheRecord(),
      storage::GetDefaultDiskInfoHelper(), std::move(callback));
}

content::GeneratedCodeCacheSettings
BisonContentBrowserClient::GetGeneratedCodeCacheSettings(
    content::BrowserContext* context) {
  // If we pass 0 for size, disk_cache will pick a default size using the
  // heuristics based on available disk size. These are implemented in
  // disk_cache::PreferredCacheSize in net/disk_cache/cache_util.cc.
  BisonBrowserContext* browser_context = static_cast<BisonBrowserContext*>(context);
  return content::GeneratedCodeCacheSettings(true, 0,
                                             browser_context->GetCacheDir());
}

void BisonContentBrowserClient::AllowCertificateError(
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    bool is_main_frame_request,
    bool strict_enforcement,
    const base::Callback<void(content::CertificateRequestResultType)>&
        callback) {
  BisonContentsClientBridge* client =
      BisonContentsClientBridge::FromWebContents(web_contents);
  bool cancel_request = true;
  if (client)
    client->AllowCertificateError(cert_error,
                                  ssl_info.cert.get(),
                                  request_url,
                                  callback,
                                  &cancel_request);
  if (cancel_request)
    callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
}

base::OnceClosure BisonContentBrowserClient::SelectClientCertificate(
    content::WebContents* web_contents,
    net::SSLCertRequestInfo* cert_request_info,
    net::ClientCertIdentityList client_certs,
    std::unique_ptr<content::ClientCertificateDelegate> delegate) {
  BisonContentsClientBridge* client =
      BisonContentsClientBridge::FromWebContents(web_contents);
  if (client)
    client->SelectClientCertificate(cert_request_info, std::move(delegate));
  return base::OnceClosure();
}

bool BisonContentBrowserClient::CanCreateWindow(
    content::RenderFrameHost* opener,
    const GURL& opener_url,
    const GURL& opener_top_level_frame_url,
    const url::Origin& source_origin,
    content::mojom::WindowContainerType container_type,
    const GURL& target_url,
    const content::Referrer& referrer,
    const std::string& frame_name,
    WindowOpenDisposition disposition,
    const blink::mojom::WindowFeatures& features,
    bool user_gesture,
    bool opener_suppressed,
    bool* no_javascript_access) {
  // We unconditionally allow popup windows at this stage and will give
  // the embedder the opporunity to handle displaying of the popup in
  // WebContentsDelegate::AddContents (via the
  // BisonContentsClient.onCreateWindow callback).
  // Note that if the embedder has blocked support for creating popup
  // windows through BisonSettings, then we won't get to this point as
  // the popup creation will have been blocked at the WebKit level.
  if (no_javascript_access) {
    *no_javascript_access = false;
  }

  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(opener);
  BisonSettings* settings = BisonSettings::FromWebContents(web_contents);

  return (settings && settings->GetJavaScriptCanOpenWindowsAutomatically()) ||
         user_gesture;
}

base::FilePath BisonContentBrowserClient::GetDefaultDownloadDirectory() {
  // Android WebView does not currently use the Chromium downloads system.
  // Download requests are cancelled immedately when recognized; see
  // BisonResourceDispatcherHost::CreateResourceHandlerForDownload. However the
  // download system still tries to start up and calls this before recognizing
  // the request has been cancelled.
  return base::FilePath();
}

std::string BisonContentBrowserClient::GetDefaultDownloadName() {
  NOTREACHED() << "Android WebView does not use chromium downloads";
  return std::string();
}

void BisonContentBrowserClient::DidCreatePpapiPlugin(
    content::BrowserPpapiHost* browser_host) {
  NOTREACHED() << "Android WebView does not support plugins";
}

bool BisonContentBrowserClient::AllowPepperSocketAPI(
    content::BrowserContext* browser_context,
    const GURL& url,
    bool private_api,
    const content::SocketPermissionRequest* params) {
  NOTREACHED() << "Android WebView does not support plugins";
  return false;
}

bool BisonContentBrowserClient::IsPepperVpnProviderAPIAllowed(
    content::BrowserContext* browser_context,
    const GURL& url) {
  NOTREACHED() << "Android WebView does not support plugins";
  return false;
}

content::TracingDelegate* BisonContentBrowserClient::GetTracingDelegate() {
  return new BisonTracingDelegate();
}

void BisonContentBrowserClient::GetAdditionalMappedFilesForChildProcess(
    const base::CommandLine& command_line,
    int child_process_id,
    content::PosixFileDescriptorInfo* mappings) {
  base::MemoryMappedFile::Region region;
  int fd = ui::GetMainAndroidPackFd(&region);
  mappings->ShareWithRegion(kAndroidWebViewMainPakDescriptor, fd, region);

  fd = ui::GetCommonResourcesPackFd(&region);
  mappings->ShareWithRegion(kAndroidWebView100PercentPakDescriptor, fd, region);

  fd = ui::GetLocalePackFd(&region);
  mappings->ShareWithRegion(kAndroidWebViewLocalePakDescriptor, fd, region);

  int crash_signal_fd =
      crashpad::CrashHandlerHost::Get()->GetDeathSignalSocket();
  if (crash_signal_fd >= 0) {
    mappings->Share(service_manager::kCrashDumpSignal, crash_signal_fd);
  }
}

void BisonContentBrowserClient::OverrideWebkitPrefs(
    content::RenderViewHost* rvh,
    content::WebPreferences* web_prefs) {
  BisonSettings* aw_settings = BisonSettings::FromWebContents(
      content::WebContents::FromRenderViewHost(rvh));
  if (aw_settings) {
    aw_settings->PopulateWebPreferences(web_prefs);
  }
}

std::vector<std::unique_ptr<content::NavigationThrottle>>
BisonContentBrowserClient::CreateThrottlesForNavigation(
    content::NavigationHandle* navigation_handle) {
  std::vector<std::unique_ptr<content::NavigationThrottle>> throttles;
  // We allow intercepting only navigations within main frames. This
  // is used to post onPageStarted. We handle shouldOverrideUrlLoading
  // via a sync IPC.
  if (navigation_handle->IsInMainFrame()) {
    // MetricsNavigationThrottle requires that it runs before
    // NavigationThrottles that may delay or cancel navigations, so only
    // NavigationThrottles that don't delay or cancel navigations (e.g.
    // throttles that are only observing callbacks without affecting navigation
    // behavior) should be added before MetricsNavigationThrottle.
    throttles.push_back(page_load_metrics::MetricsNavigationThrottle::Create(
        navigation_handle));
    // Use Synchronous mode for the navigation interceptor, since this class
    // doesn't actually call into an arbitrary client, it just posts a task to
    // call onPageStarted. shouldOverrideUrlLoading happens earlier (see
    // ContentBrowserClient::ShouldOverrideUrlLoading).
    throttles.push_back(
        navigation_interception::InterceptNavigationDelegate::CreateThrottleFor(
            navigation_handle, navigation_interception::SynchronyMode::kSync));
    throttles.push_back(std::make_unique<PolicyBlacklistNavigationThrottle>(
        navigation_handle, BisonBrowserContext::FromWebContents(
                               navigation_handle->GetWebContents())));
  }
  return throttles;
}

content::DevToolsManagerDelegate*
BisonContentBrowserClient::GetDevToolsManagerDelegate() {
  return new BisonDevToolsManagerDelegate();
}

base::Optional<service_manager::Manifest>
BisonContentBrowserClient::GetServiceManifestOverlay(base::StringPiece name) {
  if (name == content::mojom::kBrowserServiceName)
    return GetAWContentBrowserOverlayManifest();
  if (name == content::mojom::kRendererServiceName)
    return GetAWContentRendererOverlayManifest();
  return base::nullopt;
}

void BisonContentBrowserClient::BindInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  frame_interfaces_.TryBindInterface(interface_name, &interface_pipe,
                                     render_frame_host);
}

bool BisonContentBrowserClient::BindAssociatedReceiverFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedInterfaceEndpointHandle* handle) {
  if (interface_name == autofill::mojom::AutofillDriver::Name_) {
    autofill::ContentAutofillDriverFactory::BindAutofillDriver(
        mojo::PendingAssociatedReceiver<autofill::mojom::AutofillDriver>(
            std::move(*handle)),
        render_frame_host);
    return true;
  }
  if (interface_name == content_capture::mojom::ContentCaptureReceiver::Name_) {
    content_capture::ContentCaptureReceiverManager::BindContentCaptureReceiver(
        mojo::PendingAssociatedReceiver<
            content_capture::mojom::ContentCaptureReceiver>(std::move(*handle)),
        render_frame_host);
    return true;
  }

  return false;
}

void BisonContentBrowserClient::ExposeInterfacesToRenderer(
    service_manager::BinderRegistry* registry,
    blink::AssociatedInterfaceRegistry* associated_registry,
    content::RenderProcessHost* render_process_host) {
  content::ResourceContext* resource_context =
      render_process_host->GetBrowserContext()->GetResourceContext();
  registry->AddInterface(
      base::BindRepeating(
          &safe_browsing::MojoSafeBrowsingImpl::MaybeCreate,
          render_process_host->GetID(), resource_context,
          base::BindRepeating(
              &BisonContentBrowserClient::GetSafeBrowsingUrlCheckerDelegate,
              base::Unretained(this))),
      base::CreateSingleThreadTaskRunner({BrowserThread::IO}));

#if BUILDFLAG(ENABLE_SPELLCHECK)
  registry->AddInterface(
      base::BindRepeating(&SpellCheckHostImpl::Create),
      base::CreateSingleThreadTaskRunner({BrowserThread::UI}));
#endif
}

std::vector<std::unique_ptr<blink::URLLoaderThrottle>>
BisonContentBrowserClient::CreateURLLoaderThrottles(
    const network::ResourceRequest& request,
    content::BrowserContext* browser_context,
    const base::RepeatingCallback<content::WebContents*()>& wc_getter,
    content::NavigationUIData* navigation_ui_data,
    int frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  std::vector<std::unique_ptr<blink::URLLoaderThrottle>> result;

  result.push_back(safe_browsing::BrowserURLLoaderThrottle::Create(
      base::BindOnce(
          [](BisonContentBrowserClient* client, content::ResourceContext*) {
            return client->GetSafeBrowsingUrlCheckerDelegate();
          },
          base::Unretained(this)),
      wc_getter, frame_tree_node_id, browser_context->GetResourceContext()));

  if (request.resource_type ==
      static_cast<int>(content::ResourceType::kMainFrame)) {
    const bool is_load_url =
        request.transition_type & ui::PAGE_TRANSITION_FROM_API;
    const bool is_go_back_forward =
        request.transition_type & ui::PAGE_TRANSITION_FORWARD_BACK;
    const bool is_reload = ui::PageTransitionCoreTypeIs(
        static_cast<ui::PageTransition>(request.transition_type),
        ui::PAGE_TRANSITION_RELOAD);
    if (is_load_url || is_go_back_forward || is_reload) {
      result.push_back(
          std::make_unique<BisonURLLoaderThrottle>(static_cast<BisonResourceContext*>(
              browser_context->GetResourceContext())));
    }
  }

  return result;
}

scoped_refptr<safe_browsing::UrlCheckerDelegate>
BisonContentBrowserClient::GetSafeBrowsingUrlCheckerDelegate() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!safe_browsing_url_checker_delegate_) {
    safe_browsing_url_checker_delegate_ = new BisonUrlCheckerDelegateImpl(
        BisonBrowserProcess::GetInstance()->GetSafeBrowsingDBManager(),
        BisonBrowserProcess::GetInstance()->GetSafeBrowsingUIManager(),
        BisonBrowserProcess::GetInstance()->GetSafeBrowsingWhitelistManager());
  }

  return safe_browsing_url_checker_delegate_;
}

void BisonContentBrowserClient::ExposeInterfacesToMediaService(
    service_manager::BinderRegistry* registry,
    content::RenderFrameHost* render_frame_host) {
#if BUILDFLAG(ENABLE_MOJO_CDM)
  registry->AddInterface(
      base::BindRepeating(&CreateMediaDrmStorage, render_frame_host));
#endif
}

bool BisonContentBrowserClient::ShouldOverrideUrlLoading(
    int frame_tree_node_id,
    bool browser_initiated,
    const GURL& gurl,
    const std::string& request_method,
    bool has_user_gesture,
    bool is_redirect,
    bool is_main_frame,
    ui::PageTransition transition,
    bool* ignore_navigation) {
  *ignore_navigation = false;

  // Only GETs can be overridden.
  if (request_method != "GET")
    return true;

  bool application_initiated =
      browser_initiated || transition & ui::PAGE_TRANSITION_FORWARD_BACK;

  // Don't offer application-initiated navigations unless it's a redirect.
  if (application_initiated && !is_redirect)
    return true;

  // For HTTP schemes, only top-level navigations can be overridden. Similarly,
  // WebView Classic lets app override only top level about:blank navigations.
  // So we filter out non-top about:blank navigations here.
  //
  // Note: about:blank navigations are not received in this path at the moment,
  // they use the old SYNC IPC path as they are not handled by network stack.
  // However, the old path should be removed in future.
  if (!is_main_frame &&
      (gurl.SchemeIs(url::kHttpScheme) || gurl.SchemeIs(url::kHttpsScheme) ||
       gurl.SchemeIs(url::kAboutScheme)))
    return true;

  WebContents* web_contents =
      WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  if (web_contents == nullptr)
    return true;
  BisonContentsClientBridge* client_bridge =
      BisonContentsClientBridge::FromWebContents(web_contents);
  if (client_bridge == nullptr)
    return true;

  base::string16 url = base::UTF8ToUTF16(gurl.possibly_invalid_spec());
  return client_bridge->ShouldOverrideUrlLoading(
      url, has_user_gesture, is_redirect, is_main_frame, ignore_navigation);
}

bool BisonContentBrowserClient::ShouldCreateThreadPool() {
  return g_should_create_thread_pool;
}

std::unique_ptr<content::LoginDelegate>
BisonContentBrowserClient::CreateLoginDelegate(
    const net::AuthChallengeInfo& auth_info,
    content::WebContents* web_contents,
    const content::GlobalRequestID& request_id,
    bool is_main_frame,
    const GURL& url,
    scoped_refptr<net::HttpResponseHeaders> response_headers,
    bool first_auth_attempt,
    LoginAuthRequiredCallback auth_required_callback) {
  return std::make_unique<BisonHttpAuthHandler>(auth_info, web_contents,
                                             first_auth_attempt,
                                             std::move(auth_required_callback));
}

bool BisonContentBrowserClient::HandleExternalProtocol(
    const GURL& url,
    content::WebContents::Getter web_contents_getter,
    int child_id,
    content::NavigationUIData* navigation_data,
    bool is_main_frame,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    const base::Optional<url::Origin>& initiating_origin,
    network::mojom::URLLoaderFactoryPtr* out_factory) {
  mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver =
      mojo::MakeRequest(out_factory);
  if (content::BrowserThread::CurrentlyOn(content::BrowserThread::IO)) {
    // Manages its own lifetime.
    new bison::BisonProxyingURLLoaderFactory(
        0 /* process_id */, std::move(receiver), nullptr,
        true /* intercept_only */);
  } else {
    base::PostTask(
        FROM_HERE, {content::BrowserThread::IO},
        base::BindOnce(
            [](mojo::PendingReceiver<network::mojom::URLLoaderFactory>
                   receiver) {
              // Manages its own lifetime.
              new bison::BisonProxyingURLLoaderFactory(
                  0 /* process_id */, std::move(receiver), nullptr,
                  true /* intercept_only */);
            },
            std::move(receiver)));
  }
  return false;
}

void BisonContentBrowserClient::RegisterNonNetworkSubresourceURLLoaderFactories(
    int render_process_id,
    int render_frame_id,
    NonNetworkURLLoaderFactoryMap* factories) {
  WebContents* web_contents = content::WebContents::FromRenderFrameHost(
      content::RenderFrameHost::FromID(render_process_id, render_frame_id));
  BisonSettings* aw_settings = BisonSettings::FromWebContents(web_contents);

  if (aw_settings && aw_settings->GetAllowFileAccess()) {
    BisonBrowserContext* aw_browser_context =
        BisonBrowserContext::FromWebContents(web_contents);
    auto file_factory = CreateFileURLLoaderFactory(
        aw_browser_context->GetPath(),
        aw_browser_context->GetSharedCorsOriginAccessList());
    factories->emplace(url::kFileScheme, std::move(file_factory));
  }
}

bool BisonContentBrowserClient::ShouldIsolateErrorPage(bool in_main_frame) {
  return false;
}

bool BisonContentBrowserClient::ShouldEnableStrictSiteIsolation() {
  // TODO(lukasza): When/if we eventually add OOPIF support for AW we should
  // consider running AW tests with and without site-per-process (and this might
  // require returning true below).  Adding OOPIF support for AW is tracked by
  // https://crbug.com/869494.
  return false;
}

bool BisonContentBrowserClient::ShouldLockToOrigin(
    content::BrowserContext* browser_context,
    const GURL& effective_url) {
  // TODO(lukasza): https://crbug.cmo/869494: Once Android WebView supports
  // OOPIFs, we should remove this ShouldLockToOrigin overload.  Till then,
  // returning false helps avoid accidentally applying citadel-style Site
  // Isolation enforcement to Android WebView (and causing incorrect renderer
  // kills).
  return false;
}

bool BisonContentBrowserClient::WillCreateURLLoaderFactory(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* frame,
    int render_process_id,
    URLLoaderFactoryType type,
    const url::Origin& request_initiator,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory>* factory_receiver,
    mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
        header_client,
    bool* bypass_redirect_checks) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto proxied_receiver = std::move(*factory_receiver);
  network::mojom::URLLoaderFactoryPtrInfo target_factory_info;
  *factory_receiver = mojo::MakeRequest(&target_factory_info);
  int process_id =
      type == URLLoaderFactoryType::kNavigation ? 0 : render_process_id;

  // Android WebView has one non off-the-record browser context.
  base::PostTask(FROM_HERE, {content::BrowserThread::IO},
                 base::BindOnce(&BisonProxyingURLLoaderFactory::CreateProxy,
                                process_id, std::move(proxied_receiver),
                                std::move(target_factory_info)));
  return true;
}

void BisonContentBrowserClient::WillCreateURLLoaderFactoryForAppCacheSubresource(
    int render_process_id,
    mojo::PendingRemote<network::mojom::URLLoaderFactory>* pending_factory) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto pending_proxy = std::move(*pending_factory);
  mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver =
      pending_factory->InitWithNewPipeAndPassReceiver();

  base::PostTask(FROM_HERE, {content::BrowserThread::IO},
                 base::BindOnce(&BisonProxyingURLLoaderFactory::CreateProxy,
                                render_process_id, std::move(factory_receiver),
                                std::move(pending_proxy)));
}

uint32_t BisonContentBrowserClient::GetWebSocketOptions(
    content::RenderFrameHost* frame) {
  uint32_t options = network::mojom::kWebSocketOptionNone;
  if (!frame) {
    return options;
  }
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(frame);
  BisonContents* aw_contents = BisonContents::FromWebContents(web_contents);

  bool global_cookie_policy =
      BisonCookieAccessPolicy::GetInstance()->GetShouldAcceptCookies();
  bool third_party_cookie_policy = aw_contents->AllowThirdPartyCookies();
  if (!global_cookie_policy) {
    options |= network::mojom::kWebSocketOptionBlockAllCookies;
  } else if (!third_party_cookie_policy) {
    options |= network::mojom::kWebSocketOptionBlockThirdPartyCookies;
  }
  return options;
}

bool BisonContentBrowserClient::WillCreateRestrictedCookieManager(
    network::mojom::RestrictedCookieManagerRole role,
    content::BrowserContext* browser_context,
    const url::Origin& origin,
    const GURL& site_for_cookies,
    const url::Origin& top_frame_origin,
    bool is_service_worker,
    int process_id,
    int routing_id,
    mojo::PendingReceiver<network::mojom::RestrictedCookieManager>* receiver) {
  mojo::PendingReceiver<network::mojom::RestrictedCookieManager> orig_receiver =
      std::move(*receiver);

  mojo::PendingRemote<network::mojom::RestrictedCookieManager>
      target_rcm_remote;
  *receiver = target_rcm_remote.InitWithNewPipeAndPassReceiver();

  BisonProxyingRestrictedCookieManager::CreateAndBind(
      std::move(target_rcm_remote), is_service_worker, process_id, routing_id,
      std::move(orig_receiver));

  return false;  // only made a proxy, still need the actual impl to be made.
}

std::string BisonContentBrowserClient::GetProduct() {
  return bison::GetProduct();
}

std::string BisonContentBrowserClient::GetUserAgent() {
  return bison::GetUserAgent();
}

content::ContentBrowserClient::WideColorGamutHeuristic
BisonContentBrowserClient::GetWideColorGamutHeuristic() {
  if (base::FeatureList::IsEnabled(features::kWebViewWideColorGamutSupport))
    return WideColorGamutHeuristic::kUseWindow;

  if (display::Display::HasForceDisplayColorProfile() &&
      display::Display::GetForcedDisplayColorProfile() ==
          gfx::ColorSpace::CreateDisplayP3D65()) {
    return WideColorGamutHeuristic::kUseWindow;
  }

  return WideColorGamutHeuristic::kNone;
}

void BisonContentBrowserClient::LogWebFeatureForCurrentPage(
    content::RenderFrameHost* render_frame_host,
    blink::mojom::WebFeature feature) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  page_load_metrics::mojom::PageLoadFeatures new_features({feature}, {}, {});
  page_load_metrics::MetricsWebContentsObserver::RecordFeatureUsage(
      render_frame_host, new_features);
}

content::SpeechRecognitionManagerDelegate*
BisonContentBrowserClient::CreateSpeechRecognitionManagerDelegate() {
  return new BisonSpeechRecognitionManagerDelegate();
}

// static
void BisonContentBrowserClient::DisableCreatingThreadPool() {
  g_should_create_thread_pool = false;
}

}  // namespace bison