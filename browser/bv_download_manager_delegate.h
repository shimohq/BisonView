// create by jiang947

#ifndef BISON_BROWSER_BISON_DOWNLOAD_MANAGER_DELEGATE_H_
#define BISON_BROWSER_BISON_DOWNLOAD_MANAGER_DELEGATE_H_

#include <string>

#include "base/macros.h"
#include "base/supports_user_data.h"
#include "content/public/browser/download_manager_delegate.h"

namespace content {

class WebContents;

}  // namespace content

namespace bison {

// Android WebView does not use Chromium downloads, so implement methods here to
// unconditionally cancel the download.
class BvDownloadManagerDelegate : public content::DownloadManagerDelegate,
                                  public base::SupportsUserData::Data {
 public:
  BvDownloadManagerDelegate();
  ~BvDownloadManagerDelegate() override;

  // content::DownloadManagerDelegate implementation.
  bool InterceptDownloadIfApplicable(
      const GURL& url,
      const std::string& user_agent,
      const std::string& content_disposition,
      const std::string& mime_type,
      const std::string& request_origin,
      int64_t content_length,
      bool is_transient,
      content::WebContents* web_contents) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BvDownloadManagerDelegate);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_DOWNLOAD_MANAGER_DELEGATE_H_
