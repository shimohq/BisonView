#ifndef BISON_BROWSER_BV_CLIENT_HINTS_CONTROLLER_DELEGATE_H_
#define BISON_BROWSER_BV_CLIENT_HINTS_CONTROLLER_DELEGATE_H_

#include "content/public/browser/client_hints_controller_delegate.h"
#include "services/network/public/mojom/web_client_hints_types.mojom-forward.h"

namespace blink {
class EnabledClientHints;
struct UserAgentMetadata;
}  // namespace blink

namespace content {
class RenderFrameHost;
}  // namespace content

namespace gfx {
class Size;
}  // namespace gfx

namespace network {
class NetworkQualityTracker;
}  // namespace network

namespace url {
class GURL;
class Origin;
}  // namespace url

namespace bison {

class BvClientHintsControllerDelegate
    : public content::ClientHintsControllerDelegate {
 public:
  BvClientHintsControllerDelegate();
  ~BvClientHintsControllerDelegate() override;

  network::NetworkQualityTracker* GetNetworkQualityTracker() override;

  void GetAllowedClientHintsFromSource(
      const url::Origin& origin,
      blink::EnabledClientHints* client_hints) override;

  bool IsJavaScriptAllowed(const GURL& url,
                           content::RenderFrameHost* parent_rfh) override;

  bool AreThirdPartyCookiesBlocked(const GURL& url) override;

  blink::UserAgentMetadata GetUserAgentMetadata() override;

  void PersistClientHints(const url::Origin& primary_origin,
                          content::RenderFrameHost* parent_rfh,
                          const std::vector<network::mojom::WebClientHintsType>&
                              client_hints) override;

  void SetAdditionalClientHints(
      const std::vector<network::mojom::WebClientHintsType>&) override;

  void ClearAdditionalClientHints() override;

  void SetMostRecentMainFrameViewportSize(
      const gfx::Size& viewport_size) override;

  gfx::Size GetMostRecentMainFrameViewportSize() override;
};

}  // namespace bison

#endif  // BISON_BROWSER_BV_CLIENT_HINTS_CONTROLLER_DELEGATE_H_
