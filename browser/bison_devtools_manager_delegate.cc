#include "bison_devtools_manager_delegate.h"

#include <stdint.h>

#include <vector>

#include "base/atomicops.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "bison/grit/bison_resources.h"
#include "bison_contents.h"
#include "build/build_config.h"
#include "content/public/browser/android/devtools_auth.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_socket_factory.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/user_agent.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_source.h"
#include "net/socket/tcp_server_socket.h"
#include "net/socket/unix_domain_server_socket_posix.h"
#include "ui/base/resource/resource_bundle.h"

namespace bison {

namespace {

const int kBackLog = 10;

base::subtle::Atomic32 g_last_used_port;

class UnixDomainServerSocketFactory : public content::DevToolsSocketFactory {
 public:
  explicit UnixDomainServerSocketFactory(const std::string& socket_name)
      : socket_name_(socket_name) {}

 private:
  // content::DevToolsSocketFactory.
  std::unique_ptr<net::ServerSocket> CreateForHttpServer() override {
    std::unique_ptr<net::UnixDomainServerSocket> socket(
        new net::UnixDomainServerSocket(
            base::BindRepeating(&content::CanUserConnectToDevTools),
            true /* use_abstract_namespace */));
    if (socket->BindAndListen(socket_name_, kBackLog) != net::OK)
      return std::unique_ptr<net::ServerSocket>();

    return std::move(socket);
  }

  std::unique_ptr<net::ServerSocket> CreateForTethering(
      std::string* out_name) override {
    return nullptr;
  }

  std::string socket_name_;

  DISALLOW_COPY_AND_ASSIGN(UnixDomainServerSocketFactory);
};

std::unique_ptr<content::DevToolsSocketFactory> CreateSocketFactory() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  std::string socket_name = "bison_devtools_remote";
  if (command_line.HasSwitch(switches::kRemoteDebuggingSocketName)) {
    socket_name =
        command_line.GetSwitchValueASCII(switches::kRemoteDebuggingSocketName);
  }
  VLOG(0) << "remote debug socket name :" << socket_name;
  return std::unique_ptr<content::DevToolsSocketFactory>(
      new UnixDomainServerSocketFactory(socket_name));
}

}  //  namespace

// BisonDevToolsManagerDelegate ----------------------------------------------

// static
int BisonDevToolsManagerDelegate::GetHttpHandlerPort() {
  return base::subtle::NoBarrier_Load(&g_last_used_port);
}

// static
void BisonDevToolsManagerDelegate::StartHttpHandler(
    BrowserContext* browser_context) {
  std::string frontend_url;
  DevToolsAgentHost::StartRemoteDebuggingServer(
      CreateSocketFactory(), browser_context->GetPath(), base::FilePath());

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kRemoteDebuggingPipe))
    DevToolsAgentHost::StartRemoteDebuggingPipeHandler();
}

// static
void BisonDevToolsManagerDelegate::StopHttpHandler() {
  DevToolsAgentHost::StopRemoteDebuggingServer();
}

BisonDevToolsManagerDelegate::BisonDevToolsManagerDelegate() {}

BisonDevToolsManagerDelegate::~BisonDevToolsManagerDelegate() {}



std::string BisonDevToolsManagerDelegate::GetTargetDescription(
    content::WebContents* web_contents) {
  // jiang 这里貌似要自己来玩 surface;
  // android_webview::BrowserViewRenderer* bvr =
  //     android_webview::BrowserViewRenderer::FromWebContents(web_contents);
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
  return "";
}

std::string BisonDevToolsManagerDelegate::GetDiscoveryPageHTML() {
  const char html[] =
      "<html>"
      "<head><title>BisonView remote debugging</title></head>"
      "<body>Please use <a href=\'chrome://inspect\'>chrome://inspect</a>"
      "</body>"
      "</html>";
  return html;
}

bool BisonDevToolsManagerDelegate::IsBrowserTargetDiscoverable() {
  return true;
}

}  // namespace bison
