#include "bison/renderer/bison_content_renderer_client.h"

#include <string>

#include "bison/common/render_view_messages.h"
#include "bison/renderer/bison_render_view_observer.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "components/cdm/renderer/external_clear_key_key_system_properties.h"
#include "components/web_cache/renderer/web_cache_impl.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/simple_connection_filter.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "net/base/net_errors.h"
#include "ppapi/buildflags/buildflags.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_error.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_navigation_type.h"
#include "third_party/blink/public/web/web_security_policy.h"
#include "third_party/blink/public/web/web_view.h"
#include "v8/include/v8.h"

#if BUILDFLAG(ENABLE_MOJO_CDM)  // true
#include "base/feature_list.h"
#include "media/base/media_switches.h"
#endif

using content::ContentRendererClient;
using content::RenderFrame;
using content::RenderThread;
using content::RenderView;

namespace bison {

BisonContentRendererClient::BisonContentRendererClient() {}

BisonContentRendererClient::~BisonContentRendererClient() {}

void BisonContentRendererClient::RenderThreadStarted() {
  web_cache_impl_.reset(new web_cache::WebCacheImpl());
}

void BisonContentRendererClient::RenderViewCreated(
    content::RenderView* render_view) {
  new BisonRenderViewObserver(render_view);
}

bool BisonContentRendererClient::HasErrorPage(int http_status_code) {
  return http_status_code >= 400;
}

bool BisonContentRendererClient::HandleNavigation(
    RenderFrame* render_frame,
    bool is_content_initiated,
    bool render_view_was_created_by_renderer,
    blink::WebFrame* frame,
    const blink::WebURLRequest& request,
    blink::WebNavigationType type,
    blink::WebNavigationPolicy default_policy,
    bool is_redirect) {
  VLOG(0) << "HandleNavigation";
  if (!request.HttpMethod().Equals("GET"))
    return false;

  bool application_initiated =
      !is_content_initiated || type == blink::kWebNavigationTypeBackForward;

  // Don't offer application-initiated navigations unless it's a redirect.
  if (application_initiated && !is_redirect)
    return false;

  bool is_main_frame = !frame->Parent();
  const GURL& gurl = request.Url();
  // For HTTP schemes, only top-level navigations can be overridden. Similarly,
  // WebView Classic lets app override only top level about:blank navigations.
  // So we filter out non-top about:blank navigations here.
  if (!is_main_frame &&
      (gurl.SchemeIs(url::kHttpScheme) || gurl.SchemeIs(url::kHttpsScheme) ||
       gurl.SchemeIs(url::kAboutScheme)))
    return false;

  if (render_view_was_created_by_renderer) {
    return false;
  }

  bool ignore_navigation = false;
  base::string16 url = request.Url().GetString().Utf16();
  bool has_user_gesture = request.HasUserGesture();

  int render_frame_id = render_frame->GetRoutingID();
  RenderThread::Get()->Send(new BisonViewHostMsg_ShouldOverrideUrlLoading(
      render_frame_id, url, has_user_gesture, is_redirect, is_main_frame,
      &ignore_navigation));
  return ignore_navigation;
}

void BisonContentRendererClient::PrepareErrorPage(
    RenderFrame* render_frame,
    const blink::WebURLError& error,
    const std::string& http_method,
    std::string* error_html) {
  if (error_html && error_html->empty()) {
    *error_html =
        "<head><title>Error</title></head><body>Could not load the requested "
        "resource.<br/>Error code: " +
        base::NumberToString(error.reason()) +
        (error.reason() < 0 ? " (" + net::ErrorToString(error.reason()) + ")"
                            : "") +
        "</body>";
  }
}

void BisonContentRendererClient::PrepareErrorPageForHttpStatusError(
    content::RenderFrame* render_frame,
    const GURL& unreachable_url,
    const std::string& http_method,
    int http_status,
    std::string* error_html) {
  if (error_html) {
    *error_html =
        "<head><title>Error</title></head><body>Server returned HTTP status " +
        base::NumberToString(http_status) + "</body>";
  }
}

bool BisonContentRendererClient::IsPluginAllowedToUseDevChannelAPIs() {
  return false;
}

void BisonContentRendererClient::DidInitializeWorkerContextOnWorkerThread(
    v8::Local<v8::Context> context) {}

#if BUILDFLAG(ENABLE_MOJO_CDM)
void BisonContentRendererClient::AddSupportedKeySystems(
    std::vector<std::unique_ptr<media::KeySystemProperties>>* key_systems) {
  if (!base::FeatureList::IsEnabled(media::kExternalClearKeyForTesting))
    return;

  static const char kExternalClearKeyKeySystem[] =
      "org.chromium.externalclearkey";
  key_systems->emplace_back(
      new cdm::ExternalClearKeyProperties(kExternalClearKeyKeySystem));
}
#endif

}  // namespace bison
