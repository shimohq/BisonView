// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/browser/bison_cookie_access_policy.h"

#include <memory>

#include "bison/android/browser/bison_contents_io_thread_client.h"
#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/websocket_handshake_request_info.h"
#include "net/base/net_errors.h"
#include "net/base/static_cookie_policy.h"
#include "url/gurl.h"

using base::AutoLock;
using content::BrowserThread;
using content::WebSocketHandshakeRequestInfo;

namespace bison {

namespace {
base::LazyInstance<BisonCookieAccessPolicy>::Leaky g_lazy_instance;
}  // namespace

BisonCookieAccessPolicy::~BisonCookieAccessPolicy() {
}

BisonCookieAccessPolicy::BisonCookieAccessPolicy()
    : accept_cookies_(true) {
}

BisonCookieAccessPolicy* BisonCookieAccessPolicy::GetInstance() {
  return g_lazy_instance.Pointer();
}

bool BisonCookieAccessPolicy::GetShouldAcceptCookies() {
  AutoLock lock(lock_);
  return accept_cookies_;
}

void BisonCookieAccessPolicy::SetShouldAcceptCookies(bool allow) {
  AutoLock lock(lock_);
  accept_cookies_ = allow;
}

bool BisonCookieAccessPolicy::GetShouldAcceptThirdPartyCookies(
    int render_process_id,
    int render_frame_id,
    int frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::unique_ptr<BisonContentsIoThreadClient> io_thread_client =
      (frame_tree_node_id != content::RenderFrameHost::kNoFrameTreeNodeId)
          ? BisonContentsIoThreadClient::FromID(frame_tree_node_id)
          : BisonContentsIoThreadClient::FromID(render_process_id,
                                             render_frame_id);

  if (!io_thread_client) {
    return false;
  }
  return io_thread_client->ShouldAcceptThirdPartyCookies();
}

bool BisonCookieAccessPolicy::AllowCookies(const GURL& url,
                                        const GURL& first_party,
                                        int render_process_id,
                                        int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  bool third_party = GetShouldAcceptThirdPartyCookies(
      render_process_id, render_frame_id,
      content::RenderFrameHost::kNoFrameTreeNodeId);
  return CanAccessCookies(url, first_party, third_party);
}

bool BisonCookieAccessPolicy::CanAccessCookies(const GURL& url,
                                            const GURL& site_for_cookies,
                                            bool accept_third_party_cookies) {
  if (!accept_cookies_)
    return false;

  if (accept_third_party_cookies)
    return true;

  // File URLs are a special case. We want file URLs to be able to set cookies
  // but (for the purpose of cookies) Chrome considers different file URLs to
  // come from different origins so we use the 'allow all' cookie policy for
  // file URLs.
  if (url.SchemeIsFile())
    return true;

  // Otherwise, block third-party cookies.
  return net::StaticCookiePolicy(
             net::StaticCookiePolicy::BLOCK_ALL_THIRD_PARTY_COOKIES)
             .CanAccessCookies(url, site_for_cookies) == net::OK;
}

}  // namespace bison
