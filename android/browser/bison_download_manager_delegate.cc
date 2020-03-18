// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/browser/bison_download_manager_delegate.h"

#include "bison/android/browser/bison_content_browser_client.h"
#include "bison/android/browser/bison_contents_client_bridge.h"
#include "base/files/file_path.h"
#include "base/task/post_task.h"
#include "components/download/public/common/download_danger_type.h"
#include "components/download/public/common/download_item.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"

namespace bison {

namespace {

void DownloadStartingOnUIThread(content::WebContents* web_contents,
                                const GURL& url,
                                const std::string& user_agent,
                                const std::string& content_disposition,
                                const std::string& mime_type,
                                int64_t content_length) {
  BisonContentsClientBridge* client =
      BisonContentsClientBridge::FromWebContents(web_contents);
  if (!client)
    return;
  client->NewDownload(url, user_agent, content_disposition, mime_type,
                      content_length);
}

}  // namespace

BisonDownloadManagerDelegate::~BisonDownloadManagerDelegate() {}

bool BisonDownloadManagerDelegate::DetermineDownloadTarget(
    download::DownloadItem* item,
    const content::DownloadTargetCallback& callback) {
  // Note this cancel is independent of the URLRequest cancel in
  // BisonResourceDispatcherHostDelegate::DownloadStarting. The request
  // could have already finished by the time DownloadStarting is called.
  callback.Run(base::FilePath() /* Empty file path for cancel */,
               download::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
               download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS, base::FilePath(),
               download::DOWNLOAD_INTERRUPT_REASON_USER_CANCELED);
  return true;
}

bool BisonDownloadManagerDelegate::ShouldCompleteDownload(
    download::DownloadItem* item,
    base::OnceClosure complete_callback) {
  NOTREACHED();
  return true;
}

bool BisonDownloadManagerDelegate::ShouldOpenDownload(
    download::DownloadItem* item,
    const content::DownloadOpenDelayedCallback& callback) {
  NOTREACHED();
  return true;
}

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

  std::string aw_user_agent = web_contents->GetUserAgentOverride();
  if (aw_user_agent.empty()) {
    // use default user agent if nothing is provided
    aw_user_agent = user_agent.empty() ? GetUserAgent() : user_agent;
  }
  base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                 base::BindOnce(&DownloadStartingOnUIThread, web_contents, url,
                                aw_user_agent, content_disposition, mime_type,
                                content_length));
  return true;
}

void BisonDownloadManagerDelegate::GetNextId(
    const content::DownloadIdCallback& callback) {
  static uint32_t next_id = download::DownloadItem::kInvalidId + 1;
  callback.Run(next_id++);
}

}  // namespace bison
