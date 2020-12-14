

#include "bison/renderer/bison_print_render_frame_helper_delegate.h"

#include "third_party/blink/public/web/web_element.h"

namespace bison {

BisonPrintRenderFrameHelperDelegate::BisonPrintRenderFrameHelperDelegate() = default;

BisonPrintRenderFrameHelperDelegate::~BisonPrintRenderFrameHelperDelegate() = default;

blink::WebElement BisonPrintRenderFrameHelperDelegate::GetPdfElement(
    blink::WebLocalFrame* frame) {
  return blink::WebElement();
}

bool BisonPrintRenderFrameHelperDelegate::IsPrintPreviewEnabled() {
  return false;
}

bool BisonPrintRenderFrameHelperDelegate::IsScriptedPrintEnabled() {
  return false;
}

bool BisonPrintRenderFrameHelperDelegate::OverridePrint(
    blink::WebLocalFrame* frame) {
  return false;
}

}  // namespace bison
