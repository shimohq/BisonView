// create by jiang947


#ifndef BISON_RENDERER_BV_CONTENT_RENDERER_CLIENT_H_
#define BISON_RENDERER_BV_CONTENT_RENDERER_CLIENT_H_


#include <memory>
#include <string>

#include "bison/common/mojom/render_message_filter.mojom.h"
#include "bison/renderer/bv_render_thread_observer.h"

#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "components/spellcheck/spellcheck_buildflags.h"
#include "content/public/renderer/content_renderer_client.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "services/service_manager/public/cpp/local_interface_provider.h"
#include "third_party/blink/public/common/thread_safe_browser_interface_broker_proxy.h"

#if BUILDFLAG(ENABLE_SPELLCHECK)
class SpellCheck;
#endif

namespace visitedlink {
class VisitedLinkReader;
}

namespace bison {

class BvContentRendererClient : public content::ContentRendererClient,
                                   public service_manager::LocalInterfaceProvider {
 public:
  BvContentRendererClient();

  BvContentRendererClient(const BvContentRendererClient&) = delete;
  BvContentRendererClient& operator=(const BvContentRendererClient&) = delete;

  ~BvContentRendererClient() override;

  // ContentRendererClient implementation.
  void RenderThreadStarted() override;
  void ExposeInterfacesToBrowser(mojo::BinderMap* binders) override;
  void RenderFrameCreated(content::RenderFrame* render_frame) override;
  void WebViewCreated(blink::WebView* web_view) override;
  void PrepareErrorPage(content::RenderFrame* render_frame,
                        const blink::WebURLError& error,
                        const std::string& http_method,
                        content::mojom::AlternativeErrorPageOverrideInfoPtr
                            alternative_error_page_info,
                        std::string* error_html) override;
  uint64_t VisitedLinkHash(const char* canonical_url, size_t length) override;
  bool IsLinkVisited(uint64_t link_hash) override;
  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) override;
  void GetSupportedKeySystems(media::GetSupportedKeySystemsCB cb) override;

  bool HandleNavigation(content::RenderFrame* render_frame,
                        bool render_view_was_created_by_renderer,
                        blink::WebFrame* frame,
                        const blink::WebURLRequest& request,
                        blink::WebNavigationType type,
                        blink::WebNavigationPolicy default_policy,
                        bool is_redirect) override;


  visitedlink::VisitedLinkReader* visited_link_reader() {
    return visited_link_reader_.get();
  }

 private:
  // service_manager::LocalInterfaceProvider:
  void GetInterface(const std::string& name,
                    mojo::ScopedMessagePipeHandle request_handle) override;

  mojom::RenderMessageFilter* GetRenderMessageFilter();

  std::unique_ptr<BvRenderThreadObserver> bv_render_thread_observer_;
  std::unique_ptr<visitedlink::VisitedLinkReader> visited_link_reader_;

  // scoped_refptr<blink::ThreadSafeBrowserInterfaceBrokerProxy>
  //     browser_interface_broker_;

  mojo::AssociatedRemote<mojom::RenderMessageFilter> render_message_filter_;

#if BUILDFLAG(ENABLE_SPELLCHECK)
  std::unique_ptr<SpellCheck> spellcheck_;
#endif
};

}  // namespace bison

#endif  // BISON_RENDERER_BV_CONTENT_RENDERER_CLIENT_H_
