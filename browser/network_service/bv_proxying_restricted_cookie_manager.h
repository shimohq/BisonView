// create by jiang947

#ifndef BISON_BROWSER_NETWORK_SERVICE_BV_PROXYING_RESTRICTED_COOKIE_MANAGER_H_
#define BISON_BROWSER_NETWORK_SERVICE_BV_PROXYING_RESTRICTED_COOKIE_MANAGER_H_


#include <string>


#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/mojom/restricted_cookie_manager.mojom.h"

class GURL;

namespace bison {

// A RestrictedCookieManager which conditionally proxies to an underlying
// RestrictedCookieManager, first consulting WebView's cookie settings.
class BvProxyingRestrictedCookieManager
    : public network::mojom::RestrictedCookieManager {
 public:
  // Creates a BvProxyingRestrictedCookieManager that lives on IO thread,
  // binding it to handle communications from |receiver|. The requests will be
  // delegated to |underlying_rcm|. The resulting object will be owned by the
  // pipe corresponding to |request| and will in turn own |underlying_rcm|.
  //
  // Expects to be called on the UI thread.
  static void CreateAndBind(
      mojo::PendingRemote<network::mojom::RestrictedCookieManager>
          underlying_rcm,
      bool is_service_worker,
      int process_id,
      int frame_id,
      mojo::PendingReceiver<network::mojom::RestrictedCookieManager> receiver);

  BvProxyingRestrictedCookieManager(const BvProxyingRestrictedCookieManager&) =
      delete;
  BvProxyingRestrictedCookieManager& operator=(
      const BvProxyingRestrictedCookieManager&) = delete;

  ~BvProxyingRestrictedCookieManager() override;

  // network::mojom::RestrictedCookieManager interface:
  void GetAllForUrl(const GURL& url,
                    const net::SiteForCookies& site_for_cookies,
                    const url::Origin& top_frame_origin,
                    network::mojom::CookieManagerGetOptionsPtr options,
                    bool partitioned_cookies_runtime_feature_enabled,
                    GetAllForUrlCallback callback) override;
  void SetCanonicalCookie(const net::CanonicalCookie& cookie,
                          const GURL& url,
                          const net::SiteForCookies& site_for_cookies,
                          const url::Origin& top_frame_origin,
                          net::CookieInclusionStatus status,
                          SetCanonicalCookieCallback callback) override;
  void AddChangeListener(
      const GURL& url,
      const net::SiteForCookies& site_for_cookies,
      const url::Origin& top_frame_origin,
      mojo::PendingRemote<network::mojom::CookieChangeListener> listener,
      AddChangeListenerCallback callback) override;

  void SetCookieFromString(const GURL& url,
                           const net::SiteForCookies& site_for_cookies,
                           const url::Origin& top_frame_origin,
                           const std::string& cookie,
                           bool partitioned_cookies_runtime_feature_enabled,
                           SetCookieFromStringCallback callback) override;

  void GetCookiesString(const GURL& url,
                        const net::SiteForCookies& site_for_cookies,
                        const url::Origin& top_frame_origin,
                        bool partitioned_cookies_runtime_feature_enabled,
                        GetCookiesStringCallback callback) override;

  void CookiesEnabledFor(const GURL& url,
                         const net::SiteForCookies& site_for_cookies,
                         const url::Origin& top_frame_origin,
                         CookiesEnabledForCallback callback) override;

  // This one is internal.
  bool AllowCookies(const GURL& url,
                    const net::SiteForCookies& site_for_cookies) const;

  // TODO(https://crbug.com/1296161): Delete this function.
  void ConvertPartitionedCookiesToUnpartitioned(const GURL& url) override;

 private:
  BvProxyingRestrictedCookieManager(
      mojo::PendingRemote<network::mojom::RestrictedCookieManager>
          underlying_restricted_cookie_manager,
      bool is_service_worker,
      int process_id,
      int frame_id);

  static void CreateAndBindOnIoThread(
      mojo::PendingRemote<network::mojom::RestrictedCookieManager>
          underlying_rcm,
      bool is_service_worker,
      int process_id,
      int frame_id,
      mojo::PendingReceiver<network::mojom::RestrictedCookieManager> receiver);

  mojo::Remote<network::mojom::RestrictedCookieManager>
      underlying_restricted_cookie_manager_;
  bool is_service_worker_;
  int process_id_;
  int frame_id_;

  base::WeakPtrFactory<BvProxyingRestrictedCookieManager> weak_factory_{this};


};

}  // namespace bison

#endif  // BISON_BROWSER_NETWORK_SERVICE_BV_PROXYING_RESTRICTED_COOKIE_MANAGER_H_
