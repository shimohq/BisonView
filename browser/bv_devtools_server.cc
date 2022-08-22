#include "bison/browser/bv_devtools_server.h"

#include <utility>

#include "bison/browser/bv_contents.h"
#include "bison/bison_jni_headers/BvDevToolsServer_jni.h"
#include "bison/common/bv_content_client.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/browser/android/devtools_auth.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_socket_factory.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/user_agent.h"
#include "net/base/net_errors.h"
#include "net/socket/unix_domain_server_socket_posix.h"

using base::android::JavaParamRef;
using content::DevToolsAgentHost;

namespace {

const char kSocketNameFormat[] = "bisonview_devtools_remote_%d";
const char kTetheringSocketName[] = "bisonview_devtools_tethering_%d_%d";

const int kBackLog = 10;

// Factory for UnixDomainServerSocket.
class UnixDomainServerSocketFactory : public content::DevToolsSocketFactory {
 public:
  explicit UnixDomainServerSocketFactory(const std::string& socket_name)
      : socket_name_(socket_name), last_tethering_socket_(0) {}

 private:
  // content::DevToolsAgentHost::ServerSocketFactory.
  std::unique_ptr<net::ServerSocket> CreateForHttpServer() override {
    std::unique_ptr<net::UnixDomainServerSocket> socket(
        new net::UnixDomainServerSocket(
            base::BindRepeating(&content::CanUserConnectToDevTools),
            true /* use_abstract_namespace */));
    if (socket->BindAndListen(socket_name_, kBackLog) != net::OK)
      return nullptr;

    return std::move(socket);
  }

  std::unique_ptr<net::ServerSocket> CreateForTethering(
      std::string* name) override {
    *name = base::StringPrintf(kTetheringSocketName, getpid(),
                               ++last_tethering_socket_);
    std::unique_ptr<net::UnixDomainServerSocket> socket(
        new net::UnixDomainServerSocket(
            base::BindRepeating(&content::CanUserConnectToDevTools),
            true /* use_abstract_namespace */));
    if (socket->BindAndListen(*name, kBackLog) != net::OK)
      return nullptr;

    return std::move(socket);
  }

  std::string socket_name_;
  int last_tethering_socket_;


};

}  // namespace

namespace bison {

BvDevToolsServer::BvDevToolsServer() : is_started_(false) {}

BvDevToolsServer::~BvDevToolsServer() {
  Stop();
}

void BvDevToolsServer::Start() {
  if (is_started_)
    return;
  is_started_ = true;

  std::unique_ptr<content::DevToolsSocketFactory> factory(
      new UnixDomainServerSocketFactory(
          base::StringPrintf(kSocketNameFormat, getpid())));
  DevToolsAgentHost::StartRemoteDebuggingServer(
      std::move(factory),
      base::FilePath(), base::FilePath());
}

void BvDevToolsServer::Stop() {
  DevToolsAgentHost::StopRemoteDebuggingServer();
  is_started_ = false;
}

bool BvDevToolsServer::IsStarted() const {
  return is_started_;
}

static jlong JNI_BvDevToolsServer_InitRemoteDebugging(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  BvDevToolsServer* server = new BvDevToolsServer();
  return reinterpret_cast<intptr_t>(server);
}

static void JNI_BvDevToolsServer_DestroyRemoteDebugging(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jlong server) {
  delete reinterpret_cast<BvDevToolsServer*>(server);
}

static void JNI_BvDevToolsServer_SetRemoteDebuggingEnabled(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jlong server,
    jboolean enabled) {
  BvDevToolsServer* devtools_server =
      reinterpret_cast<BvDevToolsServer*>(server);
  if (enabled) {
    devtools_server->Start();
  } else {
    devtools_server->Stop();
  }
}

}  // namespace bison
