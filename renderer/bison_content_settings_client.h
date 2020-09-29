// create by jiang947 

#ifndef BISON_RENDERER_BISON_CONTENT_SETTINGS_CLIENT_H_
#define BISON_RENDERER_BISON_CONTENT_SETTINGS_CLIENT_H_


#include "base/macros.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"

namespace bison {

// Android WebView implementation of blink::WebContentSettingsClient.
class BisonContentSettingsClient : public content::RenderFrameObserver,
                                public blink::WebContentSettingsClient {
 public:
  explicit BisonContentSettingsClient(content::RenderFrame* render_view);

 private:
  ~BisonContentSettingsClient() override;

  // content::RenderFrameObserver implementation.
  void OnDestruct() override;

  // blink::WebContentSettingsClient implementation.
  bool AllowRunningInsecureContent(bool enabled_per_settings,
                                   const blink::WebSecurityOrigin& origin,
                                   const blink::WebURL& url) override;

  DISALLOW_COPY_AND_ASSIGN(BisonContentSettingsClient);
};

}  // namespace bison

#endif  // BISON_RENDERER_BISON_CONTENT_SETTINGS_CLIENT_H_
