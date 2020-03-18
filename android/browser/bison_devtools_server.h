// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_BROWSER_BISON_DEVTOOLS_SERVER_H_
#define BISON_ANDROID_BROWSER_BISON_DEVTOOLS_SERVER_H_

#include <memory>
#include <vector>

#include "base/macros.h"

namespace bison {

// This class controls WebView-specific Developer Tools remote debugging server.
class BisonDevToolsServer {
 public:
  BisonDevToolsServer();
  ~BisonDevToolsServer();

  // Opens linux abstract socket to be ready for remote debugging.
  void Start();

  // Closes debugging socket, stops debugging.
  void Stop();

  bool IsStarted() const;

 private:
  bool is_started_;
  DISALLOW_COPY_AND_ASSIGN(BisonDevToolsServer);
};

}  // namespace bison

#endif  // BISON_ANDROID_BROWSER_BISON_DEVTOOLS_SERVER_H_
