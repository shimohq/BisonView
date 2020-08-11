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
#include "bison_view.h"
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

BisonDevToolsManagerDelegate::BisonDevToolsManagerDelegate(
    BrowserContext* browser_context)
    : browser_context_(browser_context) {}

BisonDevToolsManagerDelegate::~BisonDevToolsManagerDelegate() {}

BrowserContext* BisonDevToolsManagerDelegate::GetDefaultBrowserContext() {
  return browser_context_;
}

void BisonDevToolsManagerDelegate::ClientAttached(
    content::DevToolsAgentHost* agent_host,
    content::DevToolsAgentHostClient* client) {
  // Make sure we don't receive notifications twice for the same client.
  CHECK(clients_.find(client) == clients_.end());
  clients_.insert(client);
}

void BisonDevToolsManagerDelegate::ClientDetached(
    content::DevToolsAgentHost* agent_host,
    content::DevToolsAgentHostClient* client) {
  clients_.erase(client);
}

scoped_refptr<DevToolsAgentHost> BisonDevToolsManagerDelegate::CreateNewTarget(
    const GURL& url) {
  BisonView* bisonView =
      BisonView::CreateNewWindow(browser_context_, nullptr, gfx::Size());
  bisonView->LoadURL(url);
  return DevToolsAgentHost::GetOrCreateFor(bisonView->web_contents());
}

std::string BisonDevToolsManagerDelegate::GetDiscoveryPageHTML() {
  return std::string();
}

bool BisonDevToolsManagerDelegate::HasBundledFrontendResources() {
  return false;
}

}  // namespace bison
