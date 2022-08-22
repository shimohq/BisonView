// create by jiang947

#ifndef BISON_RENDERER_BISON_PRINT_RENDER_FRAME_HELPER_DELEGATE_H_
#define BISON_RENDERER_BISON_PRINT_RENDER_FRAME_HELPER_DELEGATE_H_

#include "components/printing/renderer/print_render_frame_helper.h"

namespace bison {

class BvPrintRenderFrameHelperDelegate
    : public printing::PrintRenderFrameHelper::Delegate {
 public:
  BvPrintRenderFrameHelperDelegate();
  BvPrintRenderFrameHelperDelegate(const BvPrintRenderFrameHelperDelegate&) =
      delete;
  BvPrintRenderFrameHelperDelegate& operator=(
      const BvPrintRenderFrameHelperDelegate&) = delete;

  ~BvPrintRenderFrameHelperDelegate() override;

 private:
  // printing::PrintRenderFrameHelper::Delegate:
  blink::WebElement GetPdfElement(blink::WebLocalFrame* frame) override;
  bool IsPrintPreviewEnabled() override;
  bool IsScriptedPrintEnabled() override;
  bool OverridePrint(blink::WebLocalFrame* frame) override;
};

}  // namespace bison

#endif  // BISON_RENDERER_BISON_PRINT_RENDER_FRAME_HELPER_DELEGATE_H_
