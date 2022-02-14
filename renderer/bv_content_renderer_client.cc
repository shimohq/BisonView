#include "bison/renderer/bv_content_renderer_client.h"

#include <memory>
#include <vector>

#include "bison/common/bv_switches.h"
#include "bison/common/render_view_messages.h"
#include "bison/common/url_constants.h"
// #include "bison/grit/bison_resources.h"
// #include "bison/grit/bison_strings.h"
#include "bison/renderer/bison_content_settings_client.h"
#include "bison/renderer/bv_key_systems.h"
#include "bison/renderer/bison_print_render_frame_helper_delegate.h"
#include "bison/renderer/bison_render_frame_ext.h"
#include "bison/renderer/bv_render_view_ext.h"
// #include "bison/renderer/bison_url_loader_throttle_provider.h"
// #include "bison/renderer/bison_websocket_handshake_throttle_provider.h"
#include "bison/renderer/browser_exposed_renderer_interfaces.h"

#include "base/command_line.h"
#include "base/i18n/rtl.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "components/js_injection/renderer/js_communication.h"
#include "components/page_load_metrics/renderer/metrics_render_frame_observer.h"
#include "components/printing/renderer/print_render_frame_helper.h"
#include "components/visitedlink/renderer/visitedlink_reader.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/url_constants.h"
#include "content/public/renderer/document_state.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_navigation_type.h"
#include "third_party/blink/public/web/web_security_policy.h"
#include "url/gurl.h"
#include "url/url_constants.h"

#if BUILDFLAG(ENABLE_SPELLCHECK)
#include "components/spellcheck/renderer/spellcheck.h"
#include "components/spellcheck/renderer/spellcheck_provider.h"
#endif

using content::RenderThread;

namespace bison {

BvContentRendererClient::BvContentRendererClient() = default;

BvContentRendererClient::~BvContentRendererClient() = default;

void BvContentRendererClient::RenderThreadStarted() {
  RenderThread* thread = RenderThread::Get();
  bison_render_thread_observer_.reset(new BisonRenderThreadObserver);
  thread->AddObserver(bison_render_thread_observer_.get());

  visited_link_reader_.reset(new visitedlink::VisitedLinkReader);

  // browser_interface_broker_ =
  //     blink::Platform::Current()->GetBrowserInterfaceBroker();

#if BUILDFLAG(ENABLE_SPELLCHECK)
  if (!spellcheck_)
    spellcheck_ = std::make_unique<SpellCheck>(this);
#endif
}

void BvContentRendererClient::ExposeInterfacesToBrowser(
    mojo::BinderMap* binders) {
  // NOTE: Do not add binders directly within this method. Instead, modify the
  // definition of |ExposeRendererInterfacesToBrowser()| to ensure security
  // review coverage.
  ExposeRendererInterfacesToBrowser(this, binders);
}

bool BvContentRendererClient::HandleNavigation(
    content::RenderFrame* render_frame,
    bool is_content_initiated,
    bool render_view_was_created_by_renderer,
    blink::WebFrame* frame,
    const blink::WebURLRequest& request,
    blink::WebNavigationType type,
    blink::WebNavigationPolicy default_policy,
    bool is_redirect) {
  // Only GETs can be overridden.
  if (!request.HttpMethod().Equals("GET"))
    return false;

  // Any navigation from loadUrl, and goBack/Forward are considered application-
  // initiated and hence will not yield a shouldOverrideUrlLoading() callback.
  // Webview classic does not consider reload application-initiated so we
  // continue the same behavior.
  // TODO(sgurun) is_content_initiated is normally false for cross-origin
  // navigations but since android_webview does not swap out renderers, this
  // works fine. This will stop working if android_webview starts swapping out
  // renderers on navigation.
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

  // use NavigationInterception throttle to handle the call as that can
  // be deferred until after the java side has been constructed.
  //
  // TODO(nick): |render_view_was_created_by_renderer| was plumbed in to
  // preserve the existing code behavior, but it doesn't appear to be correct.
  // In particular, this value will be true for the initial navigation of a
  // RenderView created via window.open(), but it will also be true for all
  // subsequent navigations in that RenderView, no matter how they are
  // initiated.
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

void BvContentRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  new BisonContentSettingsClient(render_frame);
  new printing::PrintRenderFrameHelper(
      render_frame, std::make_unique<BisonPrintRenderFrameHelperDelegate>());
  new BisonRenderFrameExt(render_frame);
  new js_injection::JsCommunication(render_frame);

  // TODO(jam): when the frame tree moves into content and parent() works at
  // RenderFrame construction, simplify this by just checking parent().
  content::RenderFrame* parent_frame =
      render_frame->GetRenderView()->GetMainRenderFrame();
  if (parent_frame && parent_frame != render_frame) {
    // Avoid any race conditions from having the browser's UI thread tell the IO
    // thread that a subframe was created.
    RenderThread::Get()->Send(new BisonViewHostMsg_SubFrameCreated(
        parent_frame->GetRoutingID(), render_frame->GetRoutingID()));
  }

#if BUILDFLAG(ENABLE_SPELLCHECK)
  new SpellCheckProvider(render_frame, spellcheck_.get(), this);
#endif

}

void BvContentRendererClient::RenderViewCreated(
    content::RenderView* render_view) {
  BvRenderViewExt::RenderViewCreated(render_view);
}

bool BvContentRendererClient::HasErrorPage(int http_status_code) {
  return http_status_code >= 400;
}

bool BvContentRendererClient::ShouldSuppressErrorPage(
    content::RenderFrame* render_frame,
    const GURL& url,
    int error_code) {
  DCHECK(render_frame != nullptr);

  BisonRenderFrameExt* render_frame_ext =
      BisonRenderFrameExt::FromRenderFrame(render_frame);
  if (render_frame_ext == nullptr)
    return false;

  return render_frame_ext->GetWillSuppressErrorPage();
}

void BvContentRendererClient::PrepareErrorPage(
    content::RenderFrame* render_frame,
    const blink::WebURLError& error,
    const std::string& http_method,
    std::string* error_html) {
  // std::string err;
  // if (error.reason() == net::ERR_TEMPORARILY_THROTTLED)
  //   err = kThrottledErrorDescription;
  // else
  //   err = net::ErrorToString(error.reason());

  if (!error_html)
    return;

  // // Create the error page based on the error reason.
  // GURL gurl(error.url());
  // std::string url_string = gurl.possibly_invalid_spec();
  // int reason_id = IDS_AW_WEBPAGE_CAN_NOT_BE_LOADED;

  // if (err.empty())
  //   reason_id = IDS_AW_WEBPAGE_TEMPORARILY_DOWN;

  // std::string escaped_url = net::EscapeForHTML(url_string);
  // std::vector<std::string> replacements;
  // replacements.push_back(
  //     l10n_util::GetStringUTF8(IDS_AW_WEBPAGE_NOT_AVAILABLE));
  // replacements.push_back(
  //     l10n_util::GetStringFUTF8(reason_id, base::UTF8ToUTF16(escaped_url)));

  // // Having chosen the base reason, chose what extra information to add.
  // if (reason_id == IDS_AW_WEBPAGE_TEMPORARILY_DOWN) {
  //   replacements.push_back(
  //       l10n_util::GetStringUTF8(IDS_AW_WEBPAGE_TEMPORARILY_DOWN_SUGGESTIONS));
  // } else {
  //   replacements.push_back(err);
  // }
  // if (base::i18n::IsRTL())
  //   replacements.push_back("direction: rtl;");
  // else
  //   replacements.push_back("");
  // *error_html = base::ReplaceStringPlaceholders(
  //     ui::ResourceBundle::GetSharedInstance().DecompressDataResource(
  //         IDR_AW_LOAD_ERROR_HTML),
  //     replacements, nullptr);
}

uint64_t BvContentRendererClient::VisitedLinkHash(const char* canonical_url,
                                                  size_t length) {
  return visited_link_reader_->ComputeURLFingerprint(canonical_url, length);
}

bool BvContentRendererClient::IsLinkVisited(uint64_t link_hash) {
  return visited_link_reader_->IsVisited(link_hash);
}

void BvContentRendererClient::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
  js_injection::JsCommunication* communication =
      js_injection::JsCommunication::Get(render_frame);
  // We will get RunScriptsAtDocumentStart() event even before we received
  // RenderFrameCreated() for that |render_frame|. This is because Blink code
  // does initialization work on the main frame, which is not related to any
  // real navigation. If the communication is nullptr, it means we haven't
  // received RenderFrameCreated() yet, we simply ignore this event for
  // JsCommunication since that is not the right time to run the script and
  // the script may not reach renderer from browser yet.
  if (communication)
    communication->RunScriptsAtDocumentStart();
}

void BvContentRendererClient::AddSupportedKeySystems(
    std::vector<std::unique_ptr<::media::KeySystemProperties>>* key_systems) {
  BisonAddKeySystems(key_systems);
}

// std::unique_ptr<content::WebSocketHandshakeThrottleProvider>
// BvContentRendererClient::CreateWebSocketHandshakeThrottleProvider() {
//   return std::make_unique<BisonWebSocketHandshakeThrottleProvider>(
//       browser_interface_broker_.get());
// }

// std::unique_ptr<content::URLLoaderThrottleProvider>
// BvContentRendererClient::CreateURLLoaderThrottleProvider(
//     content::URLLoaderThrottleProviderType provider_type) {
//   return std::make_unique<BisonURLLoaderThrottleProvider>(
//       browser_interface_broker_.get(), provider_type);
// }

void BvContentRendererClient::GetInterface(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  // A dirty hack to make SpellCheckHost requests work on WebView.
  // TODO(crbug.com/806394): Use a WebView-specific service for SpellCheckHost
  // and SafeBrowsing, instead of |content_browser|.
  RenderThread::Get()->BindHostReceiver(
      mojo::GenericPendingReceiver(interface_name, std::move(interface_pipe)));
}

}  // namespace bison
