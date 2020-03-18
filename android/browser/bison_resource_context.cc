// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/browser/bison_resource_context.h"

#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

using content::BrowserThread;

namespace bison {

BisonResourceContext::BisonResourceContext() {
}

BisonResourceContext::~BisonResourceContext() {
}

void BisonResourceContext::SetExtraHeaders(
    const GURL& url, const std::string& headers) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!url.is_valid()) return;
  base::AutoLock scoped_lock(extra_headers_lock_);
  if (!headers.empty())
    extra_headers_[url.spec()] = headers;
  else
    extra_headers_.erase(url.spec());
}

std::string BisonResourceContext::GetExtraHeaders(const GURL& url) {
  if (!url.is_valid()) return std::string();
  base::AutoLock scoped_lock(extra_headers_lock_);
  std::map<std::string, std::string>::iterator iter =
      extra_headers_.find(url.spec());
  return iter != extra_headers_.end() ? iter->second : std::string();
}

}  // namespace bison
