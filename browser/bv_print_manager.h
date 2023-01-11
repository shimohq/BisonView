// create by jiang947

#ifndef BISON_BROWSER_BISON_PRINT_MANAGER_H_
#define BISON_BROWSER_BISON_PRINT_MANAGER_H_

#include <memory>

#include "components/printing/browser/print_manager.h"
#include "components/printing/common/print.mojom-forward.h"
#include "content/public/browser/web_contents_user_data.h"
#include "printing/print_settings.h"

namespace bison {

class BvPrintManager : public printing::PrintManager,
                       public content::WebContentsUserData<BvPrintManager> {
 public:
  BvPrintManager(const BvPrintManager&) = delete;
  BvPrintManager& operator=(const BvPrintManager&) = delete;

  ~BvPrintManager() override;

  static void BindPrintManagerHost(
      mojo::PendingAssociatedReceiver<printing::mojom::PrintManagerHost>
          receiver,
      content::RenderFrameHost* rfh);

  // printing::PrintManager:
  void PdfWritingDone(int page_count) override;

  bool PrintNow();

  // Updates the parameters for printing.
  void UpdateParam(std::unique_ptr<printing::PrintSettings> settings,
                 int file_descriptor,
                 PdfWritingDoneCallback callback);

 private:
  friend class content::WebContentsUserData<BvPrintManager>;

  explicit BvPrintManager(content::WebContents* contents);

  // mojom::PrintManagerHost:
  void DidPrintDocument(printing::mojom::DidPrintDocumentParamsPtr params,
                        DidPrintDocumentCallback callback) override;
  void GetDefaultPrintSettings(
      GetDefaultPrintSettingsCallback callback) override;
  void ScriptedPrint(printing::mojom::ScriptedPrintParamsPtr params,
                     ScriptedPrintCallback callback) override;

  static void OnDidPrintDocumentWritingDone(
      const PdfWritingDoneCallback& callback,
      DidPrintDocumentCallback did_print_document_cb,
      uint32_t page_count);

  std::unique_ptr<printing::PrintSettings> settings_;

  // The file descriptor into which the PDF of the document will be written.
  int fd_ = -1;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_PRINT_MANAGER_H_
