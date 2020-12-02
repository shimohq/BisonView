#include "bison/browser/bison_download_manager_delegate.h"

#include "base/files/file_path.h"
#include "base/task/post_task.h"
#include "bison/browser/bison_content_browser_client.h"
#include "bison/browser/bison_contents_client_bridge.h"
#include "components/download/public/common/download_danger_type.h"
#include "components/download/public/common/download_item.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"

namespace bison {

BisonDownloadManagerDelegate::BisonDownloadManagerDelegate() = default;
BisonDownloadManagerDelegate::~BisonDownloadManagerDelegate() {}

bool BisonDownloadManagerDelegate::InterceptDownloadIfApplicable(
    const GURL& url,
    const std::string& user_agent,
    const std::string& content_disposition,
    const std::string& mime_type,
    const std::string& request_origin,
    int64_t content_length,
    bool is_transient,
    content::WebContents* web_contents) {
  if (!web_contents)
    return false;

  // jiang 未完成 需要 ClientBridge
  return true;
}

}  // namespace bison
