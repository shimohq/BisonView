#include "bison/browser/bison_cookie_access_policy.h"

#include <memory>

#include "bison/browser/bison_contents_io_thread_client.h"

#include "base/check_op.h"
#include "base/no_destructor.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/websocket_handshake_request_info.h"
#include "net/base/net_errors.h"
#include "net/cookies/site_for_cookies.h"
#include "net/cookies/static_cookie_policy.h"
#include "url/gurl.h"

using base::AutoLock;
using content::BrowserThread;
using content::WebSocketHandshakeRequestInfo;

namespace bison {

BisonCookieAccessPolicy::~BisonCookieAccessPolicy() = default;

BisonCookieAccessPolicy::BisonCookieAccessPolicy()
    : accept_cookies_(true) {
}

BisonCookieAccessPolicy* BisonCookieAccessPolicy::GetInstance() {
  static base::NoDestructor<BisonCookieAccessPolicy> instance;
  return instance.get();
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

bool BisonCookieAccessPolicy::AllowCookies(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    int render_process_id,
    int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  bool third_party = GetShouldAcceptThirdPartyCookies(
      render_process_id, render_frame_id,
      content::RenderFrameHost::kNoFrameTreeNodeId);
  return CanAccessCookies(url, site_for_cookies, third_party);
}

bool BisonCookieAccessPolicy::CanAccessCookies(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
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
