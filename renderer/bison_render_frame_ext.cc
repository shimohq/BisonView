#include "bison/renderer/bison_render_frame_ext.h"

#include <map>
#include <memory>

#include "bison/common/bison_hit_test_data.h"
#include "bison/common/render_view_messages.h"

#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/content/renderer/autofill_agent.h"
#include "components/autofill/content/renderer/password_autofill_agent.h"
#include "components/content_capture/common/content_capture_features.h"
#include "components/content_capture/renderer/content_capture_sender.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_element_collection.h"
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_hit_test_result.h"
#include "third_party/blink/public/web/web_image_cache.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_meaningful_layout.h"
#include "third_party/blink/public/web/web_node.h"
#include "third_party/blink/public/web/web_view.h"
#include "url/url_canon.h"
#include "url/url_constants.h"
#include "url/url_util.h"

namespace bison {

namespace {

const char kAddressPrefix[] = "geo:0,0?q=";
const char kEmailPrefix[] = "mailto:";
const char kPhoneNumberPrefix[] = "tel:";

GURL GetAbsoluteUrl(const blink::WebNode& node,
                    const base::string16& url_fragment) {
  return GURL(node.GetDocument().CompleteURL(
      blink::WebString::FromUTF16(url_fragment)));
}

base::string16 GetHref(const blink::WebElement& element) {
  // Get the actual 'href' attribute, which might relative if valid or can
  // possibly contain garbage otherwise, so not using absoluteLinkURL here.
  return element.GetAttribute("href").Utf16();
}

GURL GetAbsoluteSrcUrl(const blink::WebElement& element) {
  if (element.IsNull())
    return GURL();
  return GetAbsoluteUrl(element, element.GetAttribute("src").Utf16());
}

blink::WebElement GetImgChild(const blink::WebNode& node) {
  // This implementation is incomplete (for example if is an area tag) but
  // matches the original WebViewClassic implementation.

  blink::WebElementCollection collection = node.GetElementsByHTMLTagName("img");
  DCHECK(!collection.IsNull());
  return collection.FirstItem();
}

GURL GetChildImageUrlFromElement(const blink::WebElement& element) {
  const blink::WebElement child_img = GetImgChild(element);
  if (child_img.IsNull())
    return GURL();
  return GetAbsoluteSrcUrl(child_img);
}

bool RemovePrefixAndAssignIfMatches(const base::StringPiece& prefix,
                                    const GURL& url,
                                    std::string* dest) {
  const base::StringPiece spec(url.possibly_invalid_spec());

  if (spec.starts_with(prefix)) {
    url::RawCanonOutputW<1024> output;
    url::DecodeURLEscapeSequences(
        spec.data() + prefix.length(), spec.length() - prefix.length(),
        url::DecodeURLMode::kUTF8OrIsomorphic, &output);
    *dest =
        base::UTF16ToUTF8(base::StringPiece16(output.data(), output.length()));
    return true;
  }
  return false;
}

void DistinguishAndAssignSrcLinkType(const GURL& url, BisonHitTestData* data) {
  if (RemovePrefixAndAssignIfMatches(kAddressPrefix, url,
                                     &data->extra_data_for_type)) {
    data->type = BisonHitTestData::GEO_TYPE;
  } else if (RemovePrefixAndAssignIfMatches(kPhoneNumberPrefix, url,
                                            &data->extra_data_for_type)) {
    data->type = BisonHitTestData::PHONE_TYPE;
  } else if (RemovePrefixAndAssignIfMatches(kEmailPrefix, url,
                                            &data->extra_data_for_type)) {
    data->type = BisonHitTestData::EMAIL_TYPE;
  } else {
    data->type = BisonHitTestData::SRC_LINK_TYPE;
    data->extra_data_for_type = url.possibly_invalid_spec();
    if (!data->extra_data_for_type.empty())
      data->href = base::UTF8ToUTF16(data->extra_data_for_type);
  }
}

void PopulateHitTestData(const GURL& absolute_link_url,
                         const GURL& absolute_image_url,
                         bool is_editable,
                         BisonHitTestData* data) {
  // Note: Using GURL::is_empty instead of GURL:is_valid due to the
  // WebViewClassic allowing any kind of protocol which GURL::is_valid
  // disallows. Similar reasons for using GURL::possibly_invalid_spec instead of
  // GURL::spec.
  if (!absolute_image_url.is_empty())
    data->img_src = absolute_image_url;

  const bool is_javascript_scheme =
      absolute_link_url.SchemeIs(url::kJavaScriptScheme);
  const bool has_link_url = !absolute_link_url.is_empty();
  const bool has_image_url = !absolute_image_url.is_empty();

  if (has_link_url && !has_image_url && !is_javascript_scheme) {
    DistinguishAndAssignSrcLinkType(absolute_link_url, data);
  } else if (has_link_url && has_image_url && !is_javascript_scheme) {
    data->type = BisonHitTestData::SRC_IMAGE_LINK_TYPE;
    data->extra_data_for_type = data->img_src.possibly_invalid_spec();
    if (absolute_link_url.is_valid())
      data->href = base::UTF8ToUTF16(absolute_link_url.possibly_invalid_spec());
  } else if (!has_link_url && has_image_url) {
    data->type = BisonHitTestData::IMAGE_TYPE;
    data->extra_data_for_type = data->img_src.possibly_invalid_spec();
  } else if (is_editable) {
    data->type = BisonHitTestData::EDIT_TEXT_TYPE;
    DCHECK_EQ(0u, data->extra_data_for_type.length());
  }
}

}  // namespace

// Registry for RenderFrame => BisonRenderFrameExt lookups
typedef std::map<content::RenderFrame*, BisonRenderFrameExt*> FrameExtMap;
FrameExtMap* GetFrameExtMap() {
  static base::NoDestructor<FrameExtMap> map;
  return map.get();
}

BisonRenderFrameExt::BisonRenderFrameExt(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
  // TODO(sgurun) do not create a password autofill agent (change
  // autofill agent to store a weakptr).

  // autofill::PasswordAutofillAgent* password_autofill_agent =
  //     new autofill::PasswordAutofillAgent(render_frame, &registry_);
  // new autofill::AutofillAgent(render_frame, password_autofill_agent, nullptr,
  //                             &registry_);
  // if (content_capture::features::IsContentCaptureEnabled())
  //   new content_capture::ContentCaptureSender(render_frame, &registry_);

  // Add myself to the RenderFrame => BisonRenderFrameExt register.
  GetFrameExtMap()->emplace(render_frame, this);
}

BisonRenderFrameExt::~BisonRenderFrameExt() {
  // Remove myself from the RenderFrame => BisonRenderFrameExt register.
  // Ideally, we'd just use render_frame() and erase by key. However, by this
  // time the render_frame has already been cleared so we have to iterate over
  // all render_frames in the map and wipe the one(s) that point to this
  // BisonRenderFrameExt

  auto* map = GetFrameExtMap();
  auto it = map->begin();
  while (it != map->end()) {
    if (it->second == this) {
      it = map->erase(it);
    } else {
      ++it;
    }
  }
}

BisonRenderFrameExt* BisonRenderFrameExt::FromRenderFrame(
    content::RenderFrame* render_frame) {
  DCHECK(render_frame != nullptr);
  auto iter = GetFrameExtMap()->find(render_frame);
  DCHECK(GetFrameExtMap()->end() != iter)
      << "Should always exist a render_frame_ext for a render_frame";
  BisonRenderFrameExt* render_frame_ext = iter->second;
  return render_frame_ext;
}

bool BisonRenderFrameExt::OnAssociatedInterfaceRequestForFrame(
    const std::string& interface_name,
    mojo::ScopedInterfaceEndpointHandle* handle) {
  return registry_.TryBindInterface(interface_name, handle);
}

void BisonRenderFrameExt::DidCommitProvisionalLoad(
    ui::PageTransition transition) {
  // Clear the cache when we cross site boundaries in the main frame.
  //
  // We're trying to approximate what happens with a multi-process Chromium,
  // where navigation across origins would cause a new render process to spin
  // up, and thus start with a clear cache. Wiring up a signal from browser to
  // renderer code to say "this navigation would have switched processes" would
  // be disruptive, so this clearing of the cache is the compromise.
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  if (!frame->Parent()) {
    url::Origin new_origin = url::Origin::Create(frame->GetDocument().Url());
    if (!new_origin.IsSameOriginWith(last_origin_)) {
      last_origin_ = new_origin;
      blink::WebImageCache::Clear();
    }
  }
}

bool BisonRenderFrameExt::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(BisonRenderFrameExt, message)
    IPC_MESSAGE_HANDLER(BisonViewMsg_DocumentHasImages,
                        OnDocumentHasImagesRequest)
    IPC_MESSAGE_HANDLER(BisonViewMsg_DoHitTest, OnDoHitTest)
    IPC_MESSAGE_HANDLER(BisonViewMsg_SetTextZoomFactor, OnSetTextZoomFactor)
    IPC_MESSAGE_HANDLER(BisonViewMsg_ResetScrollAndScaleState,
                        OnResetScrollAndScaleState)
    IPC_MESSAGE_HANDLER(BisonViewMsg_SetInitialPageScale, OnSetInitialPageScale)
    IPC_MESSAGE_HANDLER(BisonViewMsg_SetBackgroundColor, OnSetBackgroundColor)
    IPC_MESSAGE_HANDLER(BisonViewMsg_WillSuppressErrorPage,
                        OnSetWillSuppressErrorPage)
    IPC_MESSAGE_HANDLER(BisonViewMsg_SmoothScroll, OnSmoothScroll)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void BisonRenderFrameExt::OnDocumentHasImagesRequest(uint32_t id) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();

  // BisonViewMsg_DocumentHasImages should only be sent to the main frame.
  DCHECK(frame);
  DCHECK(!frame->Parent());

  const blink::WebElement child_img = GetImgChild(frame->GetDocument());
  bool has_images = !child_img.IsNull();

  Send(new BisonViewHostMsg_DocumentHasImagesResponse(routing_id(), id,
                                                      has_images));
}

void BisonRenderFrameExt::FocusedElementChanged(
    const blink::WebElement& element) {
  if (element.IsNull() || !render_frame() || !render_frame()->GetRenderView())
    return;

  BisonHitTestData data;

  data.href = GetHref(element);
  data.anchor_text = element.TextContent().Utf16();

  GURL absolute_link_url;
  if (element.IsLink())
    absolute_link_url = GetAbsoluteUrl(element, data.href);

  GURL absolute_image_url = GetChildImageUrlFromElement(element);

  PopulateHitTestData(absolute_link_url, absolute_image_url,
                      element.IsEditable(), &data);
  Send(new BisonViewHostMsg_UpdateHitTestData(routing_id(), data));
}

// Only main frame needs to *receive* the hit test request, because all we need
// is to get the blink::webView object and invoke a the hitTestResultForTap API
// from it.
void BisonRenderFrameExt::OnDoHitTest(const gfx::PointF& touch_center,
                                      const gfx::SizeF& touch_area) {
  blink::WebView* webview = GetWebView();
  if (!webview)
    return;

  const blink::WebHitTestResult result = webview->HitTestResultForTap(
      gfx::Point(touch_center.x(), touch_center.y()),
      blink::WebSize(touch_area.width(), touch_area.height()));
  BisonHitTestData data;

  GURL absolute_image_url = result.AbsoluteImageURL();
  if (!result.UrlElement().IsNull()) {
    data.anchor_text = result.UrlElement().TextContent().Utf16();
    data.href = GetHref(result.UrlElement());
    // If we hit an image that failed to load, Blink won't give us its URL.
    // Fall back to walking the DOM in this case.
    if (absolute_image_url.is_empty())
      absolute_image_url = GetChildImageUrlFromElement(result.UrlElement());
  }

  PopulateHitTestData(result.AbsoluteLinkURL(), absolute_image_url,
                      result.IsContentEditable(), &data);
  Send(new BisonViewHostMsg_UpdateHitTestData(routing_id(), data));
}

void BisonRenderFrameExt::OnSetTextZoomFactor(float zoom_factor) {
  DCHECK(render_frame()->IsMainFrame());

  blink::WebView* webview = GetWebView();
  if (!webview)
    return;

  // Hide selection and autofill popups.
  webview->CancelPagePopup();

  render_frame()->GetWebFrame()->FrameWidget()->SetTextZoomFactor(zoom_factor);
}

void BisonRenderFrameExt::OnResetScrollAndScaleState() {
  blink::WebView* webview = GetWebView();
  if (!webview)
    return;

  webview->ResetScrollAndScaleState();
}

void BisonRenderFrameExt::OnSetInitialPageScale(double page_scale_factor) {
  blink::WebView* webview = GetWebView();
  if (!webview)
    return;

  webview->SetInitialPageScaleOverride(page_scale_factor);
}

void BisonRenderFrameExt::OnSetBackgroundColor(SkColor c) {
  blink::WebView* webview = GetWebView();
  if (!webview)
    return;

  webview->SetBaseBackgroundColor(c);
}

void BisonRenderFrameExt::OnSmoothScroll(int target_x,
                                         int target_y,
                                         base::TimeDelta duration) {
  blink::WebView* webview = GetWebView();
  if (!webview)
    return;

  webview->SmoothScroll(target_x, target_y, duration);
}

void BisonRenderFrameExt::OnSetWillSuppressErrorPage(bool suppress) {
  this->will_suppress_error_page_ = suppress;
}

bool BisonRenderFrameExt::GetWillSuppressErrorPage() {
  return this->will_suppress_error_page_;
}

blink::WebView* BisonRenderFrameExt::GetWebView() {
  if (!render_frame() || !render_frame()->GetRenderView() ||
      !render_frame()->GetRenderView()->GetWebView())
    return nullptr;

  return render_frame()->GetRenderView()->GetWebView();
}

blink::WebFrameWidget* BisonRenderFrameExt::GetWebFrameWidget() {
  return render_frame()
             ? render_frame()->GetWebFrame()->LocalRoot()->FrameWidget()
             : nullptr;
}

void BisonRenderFrameExt::OnDestruct() {
  delete this;
}

}  // namespace bison
