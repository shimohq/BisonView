#include "bison/browser/bison_devtools_manager_delegate.h"

#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
// #include "bison/browser/gfx/browser_view_renderer.h"
#include "bison/common/bison_content_client.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/web_contents.h"

using content::DevToolsAgentHost;

namespace bison {

BisonDevToolsManagerDelegate::BisonDevToolsManagerDelegate() {}

BisonDevToolsManagerDelegate::~BisonDevToolsManagerDelegate() {}

// jiang 
std::string BisonDevToolsManagerDelegate::GetTargetDescription(
    content::WebContents* web_contents) {
  // bison::BrowserViewRenderer* bvr =
  //     bison::BrowserViewRenderer::FromWebContents(web_contents);
  // if (!bvr)
  //   return "";
  // base::DictionaryValue description;
  // description.SetBoolean("attached", bvr->attached_to_window());
  // description.SetBoolean("visible", bvr->IsVisible());
  // gfx::Rect screen_rect = bvr->GetScreenRect();
  // description.SetInteger("screenX", screen_rect.x());
  // description.SetInteger("screenY", screen_rect.y());
  // description.SetBoolean("empty", screen_rect.size().IsEmpty());
  // if (!screen_rect.size().IsEmpty()) {
  //   description.SetInteger("width", screen_rect.width());
  //   description.SetInteger("height", screen_rect.height());
  // }
  // std::string json;
  // base::JSONWriter::Write(description, &json);
  // return json;
  return "";
}

std::string BisonDevToolsManagerDelegate::GetDiscoveryPageHTML() {
  const char html[] =
      "<html>"
      "<head><title>WebView remote debugging</title></head>"
      "<body>Please use <a href=\'chrome://inspect\'>chrome://inspect</a>"
      "</body>"
      "</html>";
  return html;
}

bool BisonDevToolsManagerDelegate::IsBrowserTargetDiscoverable() {
  return true;
}

}  // namespace bison
