// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_BROWSER_BISON_BISON_ANDROID_BROWSER_TERMINATOR_H_
#define BISON_ANDROID_BROWSER_BISON_BISON_ANDROID_BROWSER_TERMINATOR_H_

#include <map>

#include "base/macros.h"
#include "components/crash/content/browser/child_exit_observer_android.h"

namespace bison {

// This class manages the browser's behavior in response to renderer exits. If
// the application does not successfully handle a renderer crash/kill, the
// browser needs to crash itself.
class BisonBrowserTerminator : public crash_reporter::ChildExitObserver::Client {
 public:
  BisonBrowserTerminator();
  ~BisonBrowserTerminator() override;

  // crash_reporter::ChildExitObserver::Client
  void OnChildExit(
      const crash_reporter::ChildExitObserver::TerminationInfo& info) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BisonBrowserTerminator);
};

}  // namespace bison

#endif  // BISON_ANDROID_BROWSER_BISON_BISON_ANDROID_BROWSER_TERMINATOR_H_
