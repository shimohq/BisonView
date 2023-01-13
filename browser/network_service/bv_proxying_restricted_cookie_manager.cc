

#include "bison/browser/network_service/bv_proxying_restricted_cookie_manager.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "bison/browser/bv_cookie_access_policy.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "url/gurl.h"

namespace bison {

class BvProxyingRestrictedCookieManagerListener
    : public network::mojom::CookieChangeListener {
 public:
  BvProxyingRestrictedCookieManagerListener(
      const GURL& url,
      const net::SiteForCookies& site_for_cookies,
      base::WeakPtr<BvProxyingRestrictedCookieManager>
          aw_restricted_cookie_manager,
      mojo::PendingRemote<network::mojom::CookieChangeListener> client_listener)
      : url_(url),
        site_for_cookies_(site_for_cookies),
        aw_restricted_cookie_manager_(aw_restricted_cookie_manager),
        client_listener_(std::move(client_listener)) {}

  void OnCookieChange(const net::CookieChangeInfo& change) override {
    if (aw_restricted_cookie_manager_ &&
        aw_restricted_cookie_manager_->AllowCookies(url_, site_for_cookies_))
      client_listener_->OnCookieChange(change);
  }

 private:
  const GURL url_;
  const net::SiteForCookies site_for_cookies_;
  base::WeakPtr<BvProxyingRestrictedCookieManager>
      aw_restricted_cookie_manager_;
  mojo::Remote<network::mojom::CookieChangeListener> client_listener_;
};

// static
void BvProxyingRestrictedCookieManager::CreateAndBind(
    mojo::PendingRemote<network::mojom::RestrictedCookieManager> underlying_rcm,
    bool is_service_worker,
    int process_id,
    int frame_id,
    mojo::PendingReceiver<network::mojom::RestrictedCookieManager> receiver) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  content::GetIOThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(
          &BvProxyingRestrictedCookieManager::CreateAndBindOnIoThread,
          std::move(underlying_rcm), is_service_worker, process_id, frame_id,
          std::move(receiver)));
}

BvProxyingRestrictedCookieManager::~BvProxyingRestrictedCookieManager() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

void BvProxyingRestrictedCookieManager::GetAllForUrl(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const url::Origin& top_frame_origin,
    network::mojom::CookieManagerGetOptionsPtr options,
    bool partitioned_cookies_runtime_feature_enabled,
    GetAllForUrlCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  if (AllowCookies(url, site_for_cookies)) {
    underlying_restricted_cookie_manager_->GetAllForUrl(
        url, site_for_cookies, top_frame_origin, std::move(options),
        partitioned_cookies_runtime_feature_enabled, std::move(callback));
  } else {
    std::move(callback).Run(std::vector<net::CookieWithAccessResult>());
  }
}

void BvProxyingRestrictedCookieManager::SetCanonicalCookie(
    const net::CanonicalCookie& cookie,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const url::Origin& top_frame_origin,
    net::CookieInclusionStatus status,
    SetCanonicalCookieCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  if (AllowCookies(url, site_for_cookies)) {
    underlying_restricted_cookie_manager_->SetCanonicalCookie(
        cookie, url, site_for_cookies, top_frame_origin, status,
        std::move(callback));
  } else {
    std::move(callback).Run(false);
  }
}

void BvProxyingRestrictedCookieManager::AddChangeListener(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const url::Origin& top_frame_origin,
    mojo::PendingRemote<network::mojom::CookieChangeListener> listener,
    AddChangeListenerCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  mojo::PendingRemote<network::mojom::CookieChangeListener>
      proxy_listener_remote;
  auto proxy_listener =
      std::make_unique<BvProxyingRestrictedCookieManagerListener>(
          url, site_for_cookies, weak_factory_.GetWeakPtr(),
          std::move(listener));

  mojo::MakeSelfOwnedReceiver(
      std::move(proxy_listener),
      proxy_listener_remote.InitWithNewPipeAndPassReceiver());

  underlying_restricted_cookie_manager_->AddChangeListener(
      url, site_for_cookies, top_frame_origin, std::move(proxy_listener_remote),
      std::move(callback));
}

void BvProxyingRestrictedCookieManager::SetCookieFromString(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const url::Origin& top_frame_origin,
    const std::string& cookie,
    bool partitioned_cookies_runtime_feature_enabled,
    SetCookieFromStringCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  if (AllowCookies(url, site_for_cookies)) {
    underlying_restricted_cookie_manager_->SetCookieFromString(
        url, site_for_cookies, top_frame_origin, cookie,
        partitioned_cookies_runtime_feature_enabled, std::move(callback));
  } else {
    std::move(callback).Run(/*site_for_cookies_ok=*/true,
                            /*top_frame_origin_ok=*/true);
  }
}

void BvProxyingRestrictedCookieManager::GetCookiesString(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const url::Origin& top_frame_origin,
    bool partitioned_cookies_runtime_feature_enabled,
    GetCookiesStringCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  if (AllowCookies(url, site_for_cookies)) {
    underlying_restricted_cookie_manager_->GetCookiesString(
        url, site_for_cookies, top_frame_origin,
        partitioned_cookies_runtime_feature_enabled, std::move(callback));
  } else {
    std::move(callback).Run("");
  }
}

void BvProxyingRestrictedCookieManager::CookiesEnabledFor(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const url::Origin& top_frame_origin,
    CookiesEnabledForCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  std::move(callback).Run(AllowCookies(url, site_for_cookies));
}

BvProxyingRestrictedCookieManager::BvProxyingRestrictedCookieManager(
    mojo::PendingRemote<network::mojom::RestrictedCookieManager>
        underlying_restricted_cookie_manager,
    bool is_service_worker,
    int process_id,
    int frame_id)
    : underlying_restricted_cookie_manager_(
          std::move(underlying_restricted_cookie_manager)),
      is_service_worker_(is_service_worker),
      process_id_(process_id),
      frame_id_(frame_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

// static
void BvProxyingRestrictedCookieManager::CreateAndBindOnIoThread(
    mojo::PendingRemote<network::mojom::RestrictedCookieManager> underlying_rcm,
    bool is_service_worker,
    int process_id,
    int frame_id,
    mojo::PendingReceiver<network::mojom::RestrictedCookieManager> receiver) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  auto wrapper = base::WrapUnique(new BvProxyingRestrictedCookieManager(
      std::move(underlying_rcm), is_service_worker, process_id, frame_id));
  mojo::MakeSelfOwnedReceiver(std::move(wrapper), std::move(receiver));
}

bool BvProxyingRestrictedCookieManager::AllowCookies(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies) const {
  if (is_service_worker_) {
    // Service worker cookies are always first-party, so only need to check
    // the global toggle.
    return BvCookieAccessPolicy::GetInstance()->GetShouldAcceptCookies();
  } else {
    return BvCookieAccessPolicy::GetInstance()->AllowCookies(
        url, site_for_cookies, process_id_, frame_id_);
  }
}

void BvProxyingRestrictedCookieManager::
    ConvertPartitionedCookiesToUnpartitioned(const GURL& url) {
  underlying_restricted_cookie_manager_
      ->ConvertPartitionedCookiesToUnpartitioned(url);
}

}  // namespace bison
