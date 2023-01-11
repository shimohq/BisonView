
#include "bison/renderer/bv_content_settings_client.h"

#include "content/public/common/url_constants.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/gurl.h"

namespace bison {

namespace {

bool AllowMixedContent(const blink::WebURL& url) {
  // We treat non-standard schemes as "secure" in the WebView to allow them to
  // be used for request interception.
  // TODO(benm): Tighten this restriction by requiring embedders to register
  // their custom schemes? See b/9420953.
  GURL gurl(url);
  return !gurl.IsStandard();
}

}  // namespace

BvContentSettingsClient::BvContentSettingsClient(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
  render_frame->GetWebFrame()->SetContentSettingsClient(this);
}

BvContentSettingsClient::~BvContentSettingsClient() {
}

bool BvContentSettingsClient::AllowImage(bool enabled_per_settings,
                                         const blink::WebURL& image_url) {
  if (ShouldAllowlistForContentSettings()) {
    return true;
  }
  return blink::WebContentSettingsClient::AllowImage(enabled_per_settings,
                                                     image_url);
}

bool BvContentSettingsClient::AllowScript(bool enabled_per_settings) {
  if (ShouldAllowlistForContentSettings()) {
    return true;
  }
  return blink::WebContentSettingsClient::AllowScript(enabled_per_settings);
}

bool BvContentSettingsClient::AllowRunningInsecureContent(
    bool enabled_per_settings,
    const blink::WebURL& url) {
  return enabled_per_settings ? true : AllowMixedContent(url);
}

bool BvContentSettingsClient::ShouldAutoupgradeMixedContent() {
  return render_frame()->GetBlinkPreferences().allow_mixed_content_upgrades;
}

void BvContentSettingsClient::OnDestruct() {
  delete this;
}

bool BvContentSettingsClient::ShouldAllowlistForContentSettings() const {
  return render_frame()->GetWebFrame()->GetDocument().Url().GetString() ==
         content::kUnreachableWebDataURL;
}
}  // namespace bison
