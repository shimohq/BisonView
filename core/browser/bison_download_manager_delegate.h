// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_BROWSER_BISON_DOWNLOAD_MANAGER_DELEGATE_H_
#define BISON_CORE_BROWSER_BISON_DOWNLOAD_MANAGER_DELEGATE_H_

#include <string>

#include "base/supports_user_data.h"
#include "content/public/browser/download_manager_delegate.h"

namespace content {

class WebContents;

}  // namespace content

namespace bison {

// Android WebView does not use Chromium downloads, so implement methods here to
// unconditionally cancel the download.
class BisonDownloadManagerDelegate : public content::DownloadManagerDelegate,
                                  public base::SupportsUserData::Data {
 public:
  ~BisonDownloadManagerDelegate() override;

  // content::DownloadManagerDelegate implementation.
  bool DetermineDownloadTarget(
      download::DownloadItem* item,
      const content::DownloadTargetCallback& callback) override;
  bool ShouldCompleteDownload(download::DownloadItem* item,
                              base::OnceClosure complete_callback) override;
  bool ShouldOpenDownload(
      download::DownloadItem* item,
      const content::DownloadOpenDelayedCallback& callback) override;
  bool InterceptDownloadIfApplicable(
      const GURL& url,
      const std::string& user_agent,
      const std::string& content_disposition,
      const std::string& mime_type,
      const std::string& request_origin,
      int64_t content_length,
      bool is_transient,
      content::WebContents* web_contents) override;
  void GetNextId(const content::DownloadIdCallback& callback) override;
};

}  // namespace bison

#endif  // BISON_CORE_BROWSER_BISON_DOWNLOAD_MANAGER_DELEGATE_H_
