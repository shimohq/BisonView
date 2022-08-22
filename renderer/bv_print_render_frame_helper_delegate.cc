

#include "bison/renderer/bv_print_render_frame_helper_delegate.h"

#include "third_party/blink/public/web/web_element.h"

namespace bison {

BvPrintRenderFrameHelperDelegate::BvPrintRenderFrameHelperDelegate() = default;

BvPrintRenderFrameHelperDelegate::~BvPrintRenderFrameHelperDelegate() = default;

blink::WebElement BvPrintRenderFrameHelperDelegate::GetPdfElement(
    blink::WebLocalFrame* frame) {
  return blink::WebElement();
}

bool BvPrintRenderFrameHelperDelegate::IsPrintPreviewEnabled() {
  return false;
}

bool BvPrintRenderFrameHelperDelegate::IsScriptedPrintEnabled() {
  return false;
}

bool BvPrintRenderFrameHelperDelegate::OverridePrint(
    blink::WebLocalFrame* frame) {
  return false;
}

}  // namespace bison
