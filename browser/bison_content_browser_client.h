// create by jiang947

#ifndef BISON_BROWSER_BISON_CONTENT_BROWSER_CLIENT_H_
#define BISON_BROWSER_BISON_CONTENT_BROWSER_CLIENT_H_

#include <stddef.h>

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "storage/browser/quota/quota_settings.h"

using content::BrowserContext;
using content::BrowserMainParts;
using content::ClientCertificateDelegate;
using content::DevToolsManagerDelegate;
using content::GeneratedCodeCacheSettings;
using content::MainFunctionParams;
using content::OpenURLParams;
using content::RenderViewHost;
using content::SiteInstance;
using content::SpeechRecognitionManagerDelegate;
using content::WebContents;
using content::WebContentsViewDelegate;
using content::WebPreferences;

namespace bison {
class BisonBrowserContext;
class BisonFeatureListCreator;

std::string GetProduct();
std::string GetUserAgent();


class BisonContentBrowserClient : public content::ContentBrowserClient {
 public:
  static std::string GetAcceptLangsImpl();

  // Sets whether the net stack should check the cleartext policy from the
  // platform. For details, see
  // https://developer.android.com/reference/android/security/NetworkSecurityPolicy.html#isCleartextTrafficPermitted().
  static void set_check_cleartext_permitted(bool permitted);
  static bool get_check_cleartext_permitted();

  explicit BisonContentBrowserClient(
      BisonFeatureListCreator* feature_list_creator);
  ~BisonContentBrowserClient() override;

  BisonBrowserContext* InitBrowserContext();

  // ContentBrowserClient overrides.
  void OnNetworkServiceCreated(
      network::mojom::NetworkService* network_service) override;
  void ConfigureNetworkContextParams(
      content::BrowserContext* context,
      bool in_memory,
      const base::FilePath& relative_partition_path,
      network::mojom::NetworkContextParams* network_context_params,
      network::mojom::CertVerifierCreationParams* cert_verifier_creation_params)
      override;
  std::unique_ptr<BrowserMainParts> CreateBrowserMainParts(
      const MainFunctionParams& parameters) override;
  content::WebContentsViewDelegate* GetWebContentsViewDelegate(
      content::WebContents* web_contents) override;    
  void RenderProcessWillLaunch(content::RenderProcessHost* host) override;
  bool IsExplicitNavigation(ui::PageTransition transition) override;
  bool IsHandledURL(const GURL& url) override;
  bool ForceSniffingFileUrlsForHtml() override;
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;
  std::string GetApplicationLocale() override;
  std::string GetAcceptLangs(BrowserContext* context) override;
  
  bool AllowAppCache(const GURL& manifest_url,
                     const GURL& site_for_cookies,
                     const base::Optional<url::Origin>& top_frame_origin,
                     content::BrowserContext* context) override;
//   scoped_refptr<content::QuotaPermissionContext> CreateQuotaPermissionContext()
//       override;
  GeneratedCodeCacheSettings GetGeneratedCodeCacheSettings(
      content::BrowserContext* context) override;
  void AllowCertificateError(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      bool is_main_frame_request,
      bool strict_enforcement,
      base::OnceCallback<void(content::CertificateRequestResultType)> callback)
      override;
  base::OnceClosure SelectClientCertificate(
      WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      net::ClientCertIdentityList client_certs,
      std::unique_ptr<ClientCertificateDelegate> delegate) override;
  bool CanCreateWindow(content::RenderFrameHost* opener,
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
                       bool* no_javascript_access) override;
  base::FilePath GetDefaultDownloadDirectory() override;
  std::string GetDefaultDownloadName() override;
  void DidCreatePpapiPlugin(content::BrowserPpapiHost* browser_host) override;
  bool AllowPepperSocketAPI(
      content::BrowserContext* browser_context,
      const GURL& url,
      bool private_api,
      const content::SocketPermissionRequest* params) override;
  bool IsPepperVpnProviderAPIAllowed(content::BrowserContext* browser_context,
                                     const GURL& url) override;     
//   content::TracingDelegate* GetTracingDelegate() override;        
  void GetAdditionalMappedFilesForChildProcess(
      const base::CommandLine& command_line,
      int child_process_id,
      content::PosixFileDescriptorInfo* mappings) override;            
  void OverrideWebkitPrefs(RenderViewHost* render_view_host,
                           WebPreferences* prefs) override;
  std::vector<std::unique_ptr<content::NavigationThrottle>>
  CreateThrottlesForNavigation(
      content::NavigationHandle* navigation_handle) override;
  DevToolsManagerDelegate* GetDevToolsManagerDelegate() override;
  base::Optional<service_manager::Manifest> GetServiceManifestOverlay(
      base::StringPiece name) override;
  bool BindAssociatedReceiverFromFrame(
      content::RenderFrameHost* render_frame_host,
      const std::string& interface_name,
      mojo::ScopedInterfaceEndpointHandle* handle) override;
  void ExposeInterfacesToRenderer(
      service_manager::BinderRegistry* registry,
      blink::AssociatedInterfaceRegistry* associated_registry,
      content::RenderProcessHost* render_process_host) override;  
//   void BindMediaServiceReceiver(content::RenderFrameHost* render_frame_host,
//                                 mojo::GenericPendingReceiver receiver) override;
  std::vector<std::unique_ptr<blink::URLLoaderThrottle>>
  CreateURLLoaderThrottles(
      const network::ResourceRequest& request,
      content::BrowserContext* browser_context,
      const base::RepeatingCallback<content::WebContents*()>& wc_getter,
      content::NavigationUIData* navigation_ui_data,
      int frame_tree_node_id) override;
  bool ShouldOverrideUrlLoading(int frame_tree_node_id,
                                bool browser_initiated,
                                const GURL& gurl,
                                const std::string& request_method,
                                bool has_user_gesture,
                                bool is_redirect,
                                bool is_main_frame,
                                ui::PageTransition transition,
                                bool* ignore_navigation) override;
  bool ShouldCreateThreadPool() override;
//   std::unique_ptr<LoginDelegate> CreateLoginDelegate(
//       const net::AuthChallengeInfo& auth_info,
//       content::WebContents* web_contents,
//       const content::GlobalRequestID& request_id,
//       bool is_main_frame,
//       const GURL& url,
//       scoped_refptr<net::HttpResponseHeaders> response_headers,
//       bool first_auth_attempt,
//       LoginAuthRequiredCallback auth_required_callback) override;
  bool HandleExternalProtocol(
      const GURL& url,
      content::WebContents::OnceGetter web_contents_getter,
      int child_id,
      content::NavigationUIData* navigation_data,
      bool is_main_frame,
      ui::PageTransition page_transition,
      bool has_user_gesture,
      const base::Optional<url::Origin>& initiating_origin,
      mojo::PendingRemote<network::mojom::URLLoaderFactory>* out_factory)
      override;
  void RegisterNonNetworkSubresourceURLLoaderFactories(
      int render_process_id,
      int render_frame_id,
      NonNetworkURLLoaderFactoryMap* factories) override;    
  bool ShouldIsolateErrorPage(bool in_main_frame) override;
  bool ShouldEnableStrictSiteIsolation() override;
  bool ShouldLockToOrigin(content::BrowserContext* browser_context,
                          const GURL& effective_url) override;
  bool WillCreateURLLoaderFactory(
      content::BrowserContext* browser_context,
      content::RenderFrameHost* frame,
      int render_process_id,
      URLLoaderFactoryType type,
      const url::Origin& request_initiator,
      base::Optional<int64_t> navigation_id,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory>* factory_receiver,
      mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
          header_client,
      bool* bypass_redirect_checks,
      bool* disable_secure_dns,
      network::mojom::URLLoaderFactoryOverridePtr* factory_override) override;
  uint32_t GetWebSocketOptions(content::RenderFrameHost* frame) override;
  bool WillCreateRestrictedCookieManager(
      network::mojom::RestrictedCookieManagerRole role,
      content::BrowserContext* browser_context,
      const url::Origin& origin,
      const net::SiteForCookies& site_for_cookies,
      const url::Origin& top_frame_origin,
      bool is_service_worker,
      int process_id,
      int routing_id,
      mojo::PendingReceiver<network::mojom::RestrictedCookieManager>* receiver)
      override;
  std::string GetProduct() override;
  std::string GetUserAgent() override;
  ContentBrowserClient::WideColorGamutHeuristic GetWideColorGamutHeuristic()
      override;
  void LogWebFeatureForCurrentPage(content::RenderFrameHost* render_frame_host,
                                   blink::mojom::WebFeature feature) override;
  bool IsOriginTrialRequiredForAppCache(
      content::BrowserContext* browser_text) override;

  BisonFeatureListCreator* bison_feature_list_creator() {
    return bison_feature_list_creator_;
  }

//   content::SpeechRecognitionManagerDelegate*
//   CreateSpeechRecognitionManagerDelegate() override;

  static void DisableCreatingThreadPool();

 private:

  std::unique_ptr<BisonBrowserContext> browser_context_;


  const bool sniff_file_urls_;

  BisonFeatureListCreator* const bison_feature_list_creator_;

  DISALLOW_COPY_AND_ASSIGN(BisonContentBrowserClient);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_CONTENT_BROWSER_CLIENT_H_
