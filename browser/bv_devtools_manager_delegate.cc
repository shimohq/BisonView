#include "bison/browser/bv_devtools_manager_delegate.h"

#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
// #include "bison/browser/gfx/browser_view_renderer.h"
#include "bison/common/bv_content_client.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/web_contents.h"

using content::DevToolsAgentHost;

namespace bison {

BvDevToolsManagerDelegate::BvDevToolsManagerDelegate() {}

BvDevToolsManagerDelegate::~BvDevToolsManagerDelegate() {}

// jiang
std::string BvDevToolsManagerDelegate::GetTargetDescription(
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

std::string BvDevToolsManagerDelegate::GetDiscoveryPageHTML() {
  const char html[] =
      "<html>"
      "<head><title>WebView remote debugging</title></head>"
      "<body>Please use <a href=\'chrome://inspect\'>chrome://inspect</a>"
      "</body>"
      "</html>";
  return html;
}

bool BvDevToolsManagerDelegate::IsBrowserTargetDiscoverable() {
  return true;
}

}  // namespace bison
