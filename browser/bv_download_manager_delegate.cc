#include "bison/browser/bv_download_manager_delegate.h"

#include "base/files/file_path.h"
#include "base/task/post_task.h"
#include "bison/browser/bv_content_browser_client.h"
#include "bison/browser/bv_contents_client_bridge.h"
#include "components/download/public/common/download_danger_type.h"
#include "components/download/public/common/download_item.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"

namespace bison {

BvDownloadManagerDelegate::BvDownloadManagerDelegate() = default;
BvDownloadManagerDelegate::~BvDownloadManagerDelegate() {}

bool BvDownloadManagerDelegate::InterceptDownloadIfApplicable(
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

  BvContentsClientBridge* client =
      BvContentsClientBridge::FromWebContents(web_contents);
  if (!client)
    return true;

  std::string bv_user_agent =
      web_contents->GetUserAgentOverride().ua_string_override;
  if (bv_user_agent.empty()) {
    // use default user agent if nothing is provided
    bv_user_agent = user_agent.empty() ? GetUserAgent() : user_agent;
  }

  client->NewDownload(url, bv_user_agent, content_disposition, mime_type,
                      content_length);
  return true;
}

}  // namespace bison
