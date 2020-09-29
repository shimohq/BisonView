// create by jiang947 


#ifndef BISON_RENDERER_BISON_PRINT_RENDER_FRAME_HELPER_DELEGATE_H_
#define BISON_RENDERER_BISON_PRINT_RENDER_FRAME_HELPER_DELEGATE_H_


#include "base/macros.h"
#include "components/printing/renderer/print_render_frame_helper.h"

namespace bison {

class BisonPrintRenderFrameHelperDelegate
    : public printing::PrintRenderFrameHelper::Delegate {
 public:
  BisonPrintRenderFrameHelperDelegate();
  ~BisonPrintRenderFrameHelperDelegate() override;

 private:
  // printing::PrintRenderFrameHelper::Delegate:
  bool CancelPrerender(content::RenderFrame* render_frame) override;
  blink::WebElement GetPdfElement(blink::WebLocalFrame* frame) override;
  bool IsPrintPreviewEnabled() override;
  bool IsScriptedPrintEnabled() override;
  bool OverridePrint(blink::WebLocalFrame* frame) override;

  DISALLOW_COPY_AND_ASSIGN(BisonPrintRenderFrameHelperDelegate);
};

}  // namespace bison

#endif  // BISON_RENDERER_BISON_PRINT_RENDER_FRAME_HELPER_DELEGATE_H_
