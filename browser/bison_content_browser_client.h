// create by jiang947

#ifndef BISON_BROWSER_BISON_CONTENT_BROWSER_CLIENT_H_
#define BISON_BROWSER_BISON_CONTENT_BROWSER_CLIENT_H_

#include <stddef.h>

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "storage/browser/quota/quota_settings.h"

using content::BrowserContext;
using content::BrowserMainParts;
using content::ClientCertificateDelegate;
using content::DevToolsManagerDelegate;
using content::GeneratedCodeCacheSettings;
using content::LoginDelegate;
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

std::string GetUserAgent();
std::string GetProduct();

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
  mojo::Remote<network::mojom::NetworkContext> CreateNetworkContext(
      BrowserContext* context,
      bool in_memory,
      const base::FilePath& relative_partition_path) override;

  std::unique_ptr<BrowserMainParts> CreateBrowserMainParts(
      const MainFunctionParams& parameters) override;

  void RenderProcessWillLaunch(content::RenderProcessHost* host) override;
  bool IsExplicitNavigation(ui::PageTransition transition) override;
  bool ShouldUseMobileFlingCurve() override;
  bool IsHandledURL(const GURL& url) override;
  void BindInterfaceRequestFromFrame(
      content::RenderFrameHost* render_frame_host,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle interface_pipe) override;
  void RunServiceInstance(
      const service_manager::Identity& identity,
      mojo::PendingReceiver<service_manager::mojom::Service>* receiver)
      override;
  bool ShouldTerminateOnServiceQuit(
      const service_manager::Identity& id) override;
  base::Optional<service_manager::Manifest> GetServiceManifestOverlay(
      base::StringPiece name) override;
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;
  std::string GetAcceptLangs(BrowserContext* context) override;
  std::string GetDefaultDownloadName() override;
  WebContentsViewDelegate* GetWebContentsViewDelegate(
      WebContents* web_contents) override;
  bool AllowAppCache(const GURL& manifest_url,
                     const GURL& first_party,
                     content::BrowserContext* context) override;
  // scoped_refptr<content::QuotaPermissionContext>
  // CreateQuotaPermissionContext()
  //     override;
  void GetQuotaSettings(
      content::BrowserContext* context,
      content::StoragePartition* partition,
      storage::OptionalQuotaSettingsCallback callback) override;
  GeneratedCodeCacheSettings GetGeneratedCodeCacheSettings(
      content::BrowserContext* context) override;
  void AllowCertificateError(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      bool is_main_frame_request,
      bool strict_enforcement,
      const base::Callback<void(content::CertificateRequestResultType)>&
          callback) override;
  base::OnceClosure SelectClientCertificate(
      WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      net::ClientCertIdentityList client_certs,
      std::unique_ptr<ClientCertificateDelegate> delegate) override;
  // SpeechRecognitionManagerDelegate* CreateSpeechRecognitionManagerDelegate()
  //     override;
  void OverrideWebkitPrefs(RenderViewHost* render_view_host,
                           WebPreferences* prefs) override;
  std::vector<std::unique_ptr<content::NavigationThrottle>>
  CreateThrottlesForNavigation(
      content::NavigationHandle* navigation_handle) override;
  DevToolsManagerDelegate* GetDevToolsManagerDelegate() override;

  std::unique_ptr<LoginDelegate> CreateLoginDelegate(
      const net::AuthChallengeInfo& auth_info,
      content::WebContents* web_contents,
      const content::GlobalRequestID& request_id,
      bool is_main_frame,
      const GURL& url,
      scoped_refptr<net::HttpResponseHeaders> response_headers,
      bool first_auth_attempt,
      LoginAuthRequiredCallback auth_required_callback) override;

  void GetAdditionalMappedFilesForChildProcess(
      const base::CommandLine& command_line,
      int child_process_id,
      content::PosixFileDescriptorInfo* mappings) override;
  std::vector<std::unique_ptr<blink::URLLoaderThrottle>>
  CreateURLLoaderThrottles(
      const network::ResourceRequest& request,
      content::BrowserContext* browser_context,
      const base::RepeatingCallback<content::WebContents*()>& wc_getter,
      content::NavigationUIData* navigation_ui_data,
      int frame_tree_node_id) override;
  void ExposeInterfacesToMediaService(
      service_manager::BinderRegistry* registry,
      content::RenderFrameHost* render_frame_host) override;
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
      mojo::PendingReceiver<network::mojom::URLLoaderFactory>* factory_receiver,
      mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
          header_client,
      bool* bypass_redirect_checks) override;
  void WillCreateURLLoaderFactoryForAppCacheSubresource(
      int render_process_id,
      mojo::PendingRemote<network::mojom::URLLoaderFactory>* pending_factory)
      override;
  uint32_t GetWebSocketOptions(content::RenderFrameHost* frame) override;
  bool WillCreateRestrictedCookieManager(
      network::mojom::RestrictedCookieManagerRole role,
      content::BrowserContext* browser_context,
      const url::Origin& origin,
      const GURL& site_for_cookies,
      const url::Origin& top_frame_origin,
      bool is_service_worker,
      int process_id,
      int routing_id,
      mojo::PendingReceiver<network::mojom::RestrictedCookieManager>* receiver)
      override;
  std::string GetUserAgent() override;
  std::string GetProduct() override;
  void LogWebFeatureForCurrentPage(content::RenderFrameHost* render_frame_host,
                                   blink::mojom::WebFeature feature) override;

  // Used for content_browsertests.
  void set_select_client_certificate_callback(
      base::OnceClosure select_client_certificate_callback) {
    select_client_certificate_callback_ =
        std::move(select_client_certificate_callback);
  }
  void set_should_terminate_on_service_quit_callback(
      base::OnceCallback<bool(const service_manager::Identity&)> callback) {
    should_terminate_on_service_quit_callback_ = std::move(callback);
  }
  void set_login_request_callback(
      base::OnceCallback<void(bool is_main_frame)> login_request_callback) {
    login_request_callback_ = std::move(login_request_callback);
  }

  BisonFeatureListCreator* bison_feature_list_creator() {
    return bison_feature_list_creator_;
  }

  static void DisableCreatingThreadPool();

 private:
  base::OnceClosure select_client_certificate_callback_;
  base::OnceCallback<bool(const service_manager::Identity&)>
      should_terminate_on_service_quit_callback_;
  base::OnceCallback<void(bool is_main_frame)> login_request_callback_;

  std::unique_ptr<BisonBrowserContext> browser_context_;

  std::unique_ptr<
      service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>>
      frame_interfaces_;

  
  BisonFeatureListCreator* const bison_feature_list_creator_;

  DISALLOW_COPY_AND_ASSIGN(BisonContentBrowserClient);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_CONTENT_BROWSER_CLIENT_H_
