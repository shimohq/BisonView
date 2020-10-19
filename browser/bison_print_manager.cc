

#include "bison/browser/bison_print_manager.h"

#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/numerics/safe_conversions.h"
#include "base/task/post_task.h"
#include "base/task/task_traits.h"
#include "components/printing/browser/print_manager_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"

namespace bison {

namespace {

int SaveDataToFd(int fd,
                 int page_count,
                 scoped_refptr<base::RefCountedSharedMemoryMapping> data) {
  bool result = fd > base::kInvalidFd &&
                base::IsValueInRangeForNumericType<int>(data->size());
  if (result) {
    int size = data->size();
    result = base::WriteFileDescriptor(fd, data->front_as<char>(), size);
  }
  return result ? page_count : 0;
}

}  // namespace

// static
BisonPrintManager* BisonPrintManager::CreateForWebContents(
    content::WebContents* contents,
    std::unique_ptr<printing::PrintSettings> settings,
    int file_descriptor,
    PrintManager::PdfWritingDoneCallback callback) {
  BisonPrintManager* print_manager = new BisonPrintManager(
      contents, std::move(settings), file_descriptor, std::move(callback));
  contents->SetUserData(UserDataKey(), base::WrapUnique(print_manager));
  return print_manager;
}

BisonPrintManager::BisonPrintManager(
    content::WebContents* contents,
    std::unique_ptr<printing::PrintSettings> settings,
    int file_descriptor,
    PdfWritingDoneCallback callback)
    : PrintManager(contents),
      settings_(std::move(settings)),
      fd_(file_descriptor) {
  DCHECK(settings_);
  pdf_writing_done_callback_ = std::move(callback);
  cookie_ = 1;  // Set a valid dummy cookie value.
}

BisonPrintManager::~BisonPrintManager() = default;

void BisonPrintManager::PdfWritingDone(int page_count) {
  if (pdf_writing_done_callback_)
    pdf_writing_done_callback_.Run(page_count);
  // Invalidate the file descriptor so it doesn't get reused.
  fd_ = -1;
}

bool BisonPrintManager::PrintNow() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* rfh = web_contents()->GetMainFrame();
  return rfh->Send(new PrintMsg_PrintPages(rfh->GetRoutingID()));
}

void BisonPrintManager::OnGetDefaultPrintSettings(
    content::RenderFrameHost* render_frame_host,
    IPC::Message* reply_msg) {
  // Unlike the printing_message_filter, we do process this in UI thread.
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  PrintMsg_Print_Params params;
  printing::RenderParamsFromPrintSettings(*settings_, &params);
  params.document_cookie = cookie_;
  PrintHostMsg_GetDefaultPrintSettings::WriteReplyParams(reply_msg, params);
  render_frame_host->Send(reply_msg);
}

void BisonPrintManager::OnScriptedPrint(
    content::RenderFrameHost* render_frame_host,
    const PrintHostMsg_ScriptedPrint_Params& scripted_params,
    IPC::Message* reply_msg) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  PrintMsg_PrintPages_Params params;
  printing::RenderParamsFromPrintSettings(*settings_, &params.params);
  params.params.document_cookie = scripted_params.cookie;
  params.pages = printing::PageRange::GetPages(settings_->ranges());
  PrintHostMsg_ScriptedPrint::WriteReplyParams(reply_msg, params);
  render_frame_host->Send(reply_msg);
}

void BisonPrintManager::OnDidPrintDocument(
    content::RenderFrameHost* render_frame_host,
    const PrintHostMsg_DidPrintDocument_Params& params,
    std::unique_ptr<DelayedFrameDispatchHelper> helper) {
  if (params.document_cookie != cookie_)
    return;

  const PrintHostMsg_DidPrintContent_Params& content = params.content;
  if (!content.metafile_data_region.IsValid()) {
    NOTREACHED() << "invalid memory handle";
    web_contents()->Stop();
    PdfWritingDone(0);
    return;
  }

  auto data = base::RefCountedSharedMemoryMapping::CreateFromWholeRegion(
      content.metafile_data_region);
  if (!data) {
    NOTREACHED() << "couldn't map";
    web_contents()->Stop();
    PdfWritingDone(0);
    return;
  }

  DCHECK(pdf_writing_done_callback_);
  base::PostTaskAndReplyWithResult(
      base::CreateTaskRunner({base::ThreadPool(), base::MayBlock(),
                              base::TaskPriority::BEST_EFFORT,
                              base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})
          .get(),
      FROM_HERE, base::BindOnce(&SaveDataToFd, fd_, number_pages_, data),
      base::BindOnce(&BisonPrintManager::OnDidPrintDocumentWritingDone,
                     pdf_writing_done_callback_, std::move(helper)));
}

// static
void BisonPrintManager::OnDidPrintDocumentWritingDone(
    const PdfWritingDoneCallback& callback,
    std::unique_ptr<DelayedFrameDispatchHelper> helper,
    int page_count) {
  if (callback)
    callback.Run(page_count);
  helper->SendCompleted();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BisonPrintManager)

}  // namespace bison
