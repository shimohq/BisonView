#include "bison/browser/bv_devtools_manager_delegate.h"

// #include "bison/browser/gfx/browser_view_renderer.h"
#include "bison/common/bv_content_client.h"
#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/web_contents.h"

using content::DevToolsAgentHost;

namespace bison {

BvDevToolsManagerDelegate::BvDevToolsManagerDelegate() {}

BvDevToolsManagerDelegate::~BvDevToolsManagerDelegate() {}

// jiang
std::string BvDevToolsManagerDelegate::GetTargetDescription(
    content::WebContents* web_contents) {
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
