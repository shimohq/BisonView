// create by jiang947

#ifndef BISON_BROWSER_BISON_BROWSER_TERMINATOR_H_
#define BISON_BROWSER_BISON_BROWSER_TERMINATOR_H_

#include <map>

#include "components/crash/content/browser/child_exit_observer_android.h"

namespace bison {

// This class manages the browser's behavior in response to renderer exits. If
// the application does not successfully handle a renderer crash/kill, the
// browser needs to crash itself.
class BvBrowserTerminator : public crash_reporter::ChildExitObserver::Client {
 public:
  BvBrowserTerminator();

  BvBrowserTerminator(const BvBrowserTerminator&) = delete;
  BvBrowserTerminator& operator=(const BvBrowserTerminator&) = delete;

  ~BvBrowserTerminator() override;

  // crash_reporter::ChildExitObserver::Client
  void OnChildExit(
      const crash_reporter::ChildExitObserver::TerminationInfo& info) override;
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_BROWSER_TERMINATOR_H_
