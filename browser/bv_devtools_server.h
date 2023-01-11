// create by jiang947

#ifndef BISON_BROWSER_BISON_DEVTOOLS_SERVER_H_
#define BISON_BROWSER_BISON_DEVTOOLS_SERVER_H_

#include <memory>
#include <vector>

namespace bison {

// This class controls BisonView-specific Developer Tools remote debugging
// server.
class BvDevToolsServer {
 public:
  BvDevToolsServer();

  BvDevToolsServer(const BvDevToolsServer&) = delete;
  BvDevToolsServer& operator=(const BvDevToolsServer&) = delete;

  ~BvDevToolsServer();

  // Opens linux abstract socket to be ready for remote debugging.
  void Start();

  // Closes debugging socket, stops debugging.
  void Stop();

  bool IsStarted() const;

 private:
  bool is_started_;
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_DEVTOOLS_SERVER_H_
