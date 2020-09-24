// create by jiang947

#ifndef BISON_BROWSER_BISON_CONTENT_BROWSER_CLIENT_H_
#define BISON_BROWSER_BISON_CONTENT_BROWSER_CLIENT_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "content/public/browser/content_browser_client.h"
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
class BisonBrowserMainParts;

std::string GetUserAgent();
std::string GetProduct();

class BisonContentBrowserClient : public content::ContentBrowserClient {
 public:
  // Gets the current instance.
  static BisonContentBrowserClient* Get();

  BisonContentBrowserClient();
  ~BisonContentBrowserClient() override;

  BisonBrowserContext* InitBrowserContext();

  // ContentBrowserClient overrides.
  std::unique_ptr<BrowserMainParts> CreateBrowserMainParts(
      const MainFunctionParams& parameters) override;
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
  // scoped_refptr<content::QuotaPermissionContext>
  // CreateQuotaPermissionContext()
  //     override;
  void GetQuotaSettings(
      content::BrowserContext* context,
      content::StoragePartition* partition,
      storage::OptionalQuotaSettingsCallback callback) override;
  GeneratedCodeCacheSettings GetGeneratedCodeCacheSettings(
      content::BrowserContext* context) override;
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

  std::string GetUserAgent() override;
  std::string GetProduct() override;

  void GetAdditionalMappedFilesForChildProcess(
      const base::CommandLine& command_line,
      int child_process_id,
      content::PosixFileDescriptorInfo* mappings) override;

  mojo::Remote<network::mojom::NetworkContext> CreateNetworkContext(
      BrowserContext* context,
      bool in_memory,
      const base::FilePath& relative_partition_path) override;

  bool ShouldOverrideUrlLoading(int frame_tree_node_id,
                                bool browser_initiated,
                                const GURL& gurl,
                                const std::string& request_method,
                                bool has_user_gesture,
                                bool is_redirect,
                                bool is_main_frame,
                                ui::PageTransition transition,
                                bool* ignore_navigation) override;
  bool WillCreateURLLoaderFactory(
      content::BrowserContext* browser_context,
      content::RenderFrameHost* frame,
      int render_process_id,
      URLLoaderFactoryType type,
      const url::Origin& request_initiator,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory>*
      factory_receiver,
      mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
          header_client,
      bool* bypass_redirect_checks) override;

  // BisonBrowserContext* browser_context();
  
  BisonBrowserMainParts* shell_browser_main_parts() {
    return shell_browser_main_parts_;
  }

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

 protected:
  virtual void ExposeInterfacesToFrame(
      service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>*
          registry);

  void set_browser_main_parts(BisonBrowserMainParts* parts) {
    shell_browser_main_parts_ = parts;
  }

 private:
  base::OnceClosure select_client_certificate_callback_;
  base::OnceCallback<bool(const service_manager::Identity&)>
      should_terminate_on_service_quit_callback_;
  base::OnceCallback<void(bool is_main_frame)> login_request_callback_;

  std::unique_ptr<
      service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>>
      frame_interfaces_;

  // Owned by content::BrowserMainLoop.
  BisonBrowserMainParts* shell_browser_main_parts_;

  std::unique_ptr<BisonBrowserContext> browser_context_;


};

// The delay for sending reports when running with --run-web-tests
constexpr base::TimeDelta kReportingDeliveryIntervalTimeForWebTests =
    base::TimeDelta::FromMilliseconds(100);

}  // namespace bison

#endif  // BISON_BROWSER_BISON_CONTENT_BROWSER_CLIENT_H_
