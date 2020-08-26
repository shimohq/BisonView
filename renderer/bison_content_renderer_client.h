// create by jiang947

#ifndef BISON_RENDERER_BISON_CONTENT_RENDERER_CLIENT_H_
#define BISON_RENDERER_BISON_CONTENT_RENDERER_CLIENT_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "build/build_config.h"
#include "content/public/renderer/content_renderer_client.h"
#include "media/mojo/buildflags.h"

namespace web_cache {
class WebCacheImpl;
}
using content::ContentRendererClient;
using content::RenderFrame;

namespace bison {

class BisonContentRendererClient : public ContentRendererClient {
 public:
  BisonContentRendererClient();
  ~BisonContentRendererClient() override;

  // ContentRendererClient implementation.
  void RenderThreadStarted() override;
  void RenderViewCreated(content::RenderView* render_view) override;
  bool HasErrorPage(int http_status_code) override;

  bool HandleNavigation(RenderFrame* render_frame,
                        bool is_content_initiated,
                        bool render_view_was_created_by_renderer,
                        blink::WebFrame* frame,
                        const blink::WebURLRequest& request,
                        blink::WebNavigationType type,
                        blink::WebNavigationPolicy default_policy,
                        bool is_redirect) override;

  void PrepareErrorPage(RenderFrame* render_frame,
                        const blink::WebURLError& error,
                        const std::string& http_method,
                        std::string* error_html) override;
  void PrepareErrorPageForHttpStatusError(content::RenderFrame* render_frame,
                                          const GURL& unreachable_url,
                                          const std::string& http_method,
                                          int http_status,
                                          std::string* error_html) override;

  // TODO(mkwst): These toggle based on the kEnablePepperTesting flag. Do we
  // need that outside of web tests?
  bool IsPluginAllowedToUseDevChannelAPIs() override;

  void DidInitializeWorkerContextOnWorkerThread(
      v8::Local<v8::Context> context) override;

#if BUILDFLAG(ENABLE_MOJO_CDM)
  void AddSupportedKeySystems(
      std::vector<std::unique_ptr<media::KeySystemProperties>>* key_systems)
      override;
#endif

 private:
  std::unique_ptr<web_cache::WebCacheImpl> web_cache_impl_;
};

}  // namespace bison

#endif  // BISON_RENDERER_BISON_CONTENT_RENDERER_CLIENT_H_
