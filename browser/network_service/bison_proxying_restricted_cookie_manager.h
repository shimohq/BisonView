// create by jiang947 


#ifndef BISON_BROWSER_NETWORK_SERVICE_BISON_PROXYING_RESTRICTED_COOKIE_MANAGER_H_
#define BISON_BROWSER_NETWORK_SERVICE_BISON_PROXYING_RESTRICTED_COOKIE_MANAGER_H_


#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/mojom/restricted_cookie_manager.mojom.h"

class GURL;

namespace bison {

// A RestrictedCookieManager which conditionally proxies to an underlying
// RestrictedCookieManager, first consulting WebView's cookie settings.
class BisonProxyingRestrictedCookieManager
    : public network::mojom::RestrictedCookieManager {
 public:
  // Creates a BisonProxyingRestrictedCookieManager that lives on IO thread,
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

  ~BisonProxyingRestrictedCookieManager() override;

  // network::mojom::RestrictedCookieManager interface:
  void GetAllForUrl(const GURL& url,
                    const GURL& site_for_cookies,
                    const url::Origin& top_frame_origin,
                    network::mojom::CookieManagerGetOptionsPtr options,
                    GetAllForUrlCallback callback) override;
  void SetCanonicalCookie(const net::CanonicalCookie& cookie,
                          const GURL& url,
                          const GURL& site_for_cookies,
                          const url::Origin& top_frame_origin,
                          SetCanonicalCookieCallback callback) override;
  void AddChangeListener(
      const GURL& url,
      const GURL& site_for_cookies,
      const url::Origin& top_frame_origin,
      mojo::PendingRemote<network::mojom::CookieChangeListener> listener,
      AddChangeListenerCallback callback) override;

  void SetCookieFromString(const GURL& url,
                           const GURL& site_for_cookies,
                           const url::Origin& top_frame_origin,
                           const std::string& cookie,
                           SetCookieFromStringCallback callback) override;

  void GetCookiesString(const GURL& url,
                        const GURL& site_for_cookies,
                        const url::Origin& top_frame_origin,
                        GetCookiesStringCallback callback) override;

  void CookiesEnabledFor(const GURL& url,
                         const GURL& site_for_cookies,
                         const url::Origin& top_frame_origin,
                         CookiesEnabledForCallback callback) override;

  // This one is internal.
  bool AllowCookies(const GURL& url, const GURL& site_for_cookies) const;

 private:
  BisonProxyingRestrictedCookieManager(
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

  base::WeakPtrFactory<BisonProxyingRestrictedCookieManager> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(BisonProxyingRestrictedCookieManager);
};

}  // namespace bison

#endif  // BISON_BROWSER_NETWORK_SERVICE_BISON_PROXYING_RESTRICTED_COOKIE_MANAGER_H_
