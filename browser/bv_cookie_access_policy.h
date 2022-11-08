// create by jiang947

#ifndef BISON_BROWSER_BISON_COOKIE_ACCESS_POLICY_H_
#define BISON_BROWSER_BISON_COOKIE_ACCESS_POLICY_H_

#include "base/lazy_instance.h"

#include "base/no_destructor.h"
#include "base/synchronization/lock.h"
#include "net/cookies/site_for_cookies.h"

class GURL;

namespace net {
class URLRequest;
}  // namespace net

namespace bison {

// Manages the cookie access (both setting and getting) policy for WebView.
// Currently we don't distinguish between sources (i.e. network vs. JavaScript)
// or between reading vs. writing cookies.
class BvCookieAccessPolicy {
 public:
  static BvCookieAccessPolicy* GetInstance();


  BvCookieAccessPolicy(const BvCookieAccessPolicy&) = delete;
  BvCookieAccessPolicy& operator=(const BvCookieAccessPolicy&) = delete;

  // Can we read/write any cookies? Can be called from any thread.
  bool GetShouldAcceptCookies();
  void SetShouldAcceptCookies(bool allow);

  // Can we read/write third party cookies?
  // |render_process_id| and |render_frame_id| must be valid.
  // Navigation requests are not associated with a renderer process. In this
  // case, |frame_tree_node_id| must be valid instead. Can only be called from
  // the IO thread.
  bool GetShouldAcceptThirdPartyCookies(int render_process_id,
                                        int render_frame_id,
                                        int frame_tree_node_id);

  // Whether or not to allow cookies for requests with these parameters.
  bool AllowCookies(const GURL& url,
                    const net::SiteForCookies& site_for_cookies,
                    int render_process_id,
                    int render_frame_id);

 private:
  friend class base::NoDestructor<BvCookieAccessPolicy>;
  friend class BisonCookieAccessPolicyTest;

  BvCookieAccessPolicy();
  ~BvCookieAccessPolicy();

  bool CanAccessCookies(const GURL& url,
                        const net::SiteForCookies& site_for_cookies,
                        bool accept_third_party_cookies);
  bool accept_cookies_;
  base::Lock lock_;
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_COOKIE_ACCESS_POLICY_H_
