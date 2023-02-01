#include "bison/browser/bv_download_manager_delegate.h"

#include "bison/browser/bv_content_browser_client.h"
#include "bison/browser/bv_contents_client_bridge.h"


#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"

namespace bison {

BvDownloadManagerDelegate::BvDownloadManagerDelegate() = default;
BvDownloadManagerDelegate::~BvDownloadManagerDelegate() = default;

bool BvDownloadManagerDelegate::InterceptDownloadIfApplicable(
    const GURL& url,
    const std::string& user_agent,
    const std::string& content_disposition,
    const std::string& mime_type,
    const std::string& request_origin,
    int64_t content_length,
    bool is_transient,
    content::WebContents* web_contents) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!web_contents)
    return true;

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
