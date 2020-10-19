// create by jiang947 

#ifndef BISON_BROWSER_BISON_PRINT_MANAGER_H_
#define BISON_BROWSER_BISON_PRINT_MANAGER_H_

#include <memory>

#include "base/macros.h"
#include "components/printing/browser/print_manager.h"
#include "components/printing/common/print_messages.h"
#include "content/public/browser/web_contents_user_data.h"
#include "printing/print_settings.h"

namespace bison {

class BisonPrintManager : public printing::PrintManager,
    public content::WebContentsUserData<BisonPrintManager> {
 public:
  // Creates an BisonPrintManager for the provided WebContents. If the
  // BisonPrintManager already exists, it is destroyed and a new one is created.
  // The returned pointer is owned by |contents|.
  static BisonPrintManager* CreateForWebContents(
      content::WebContents* contents,
      std::unique_ptr<printing::PrintSettings> settings,
      int file_descriptor,
      PdfWritingDoneCallback callback);

  ~BisonPrintManager() override;

  // printing::PrintManager:
  void PdfWritingDone(int page_count) override;

  bool PrintNow();

 private:
  friend class content::WebContentsUserData<BisonPrintManager>;

  BisonPrintManager(content::WebContents* contents,
                 std::unique_ptr<printing::PrintSettings> settings,
                 int file_descriptor,
                 PdfWritingDoneCallback callback);

  // printing::PrintManager:
  void OnDidPrintDocument(
      content::RenderFrameHost* render_frame_host,
      const PrintHostMsg_DidPrintDocument_Params& params,
      std::unique_ptr<DelayedFrameDispatchHelper> helper) override;
  void OnGetDefaultPrintSettings(content::RenderFrameHost* render_frame_host,
                                 IPC::Message* reply_msg) override;
  void OnScriptedPrint(content::RenderFrameHost* render_frame_host,
                       const PrintHostMsg_ScriptedPrint_Params& params,
                       IPC::Message* reply_msg) override;

  static void OnDidPrintDocumentWritingDone(
      const PdfWritingDoneCallback& callback,
      std::unique_ptr<DelayedFrameDispatchHelper> helper,
      int page_count);

  const std::unique_ptr<printing::PrintSettings> settings_;

  // The file descriptor into which the PDF of the document will be written.
  int fd_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();

  DISALLOW_COPY_AND_ASSIGN(BisonPrintManager);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_PRINT_MANAGER_H_
