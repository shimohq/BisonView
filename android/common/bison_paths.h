// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_COMMON_BISON_PATHS_H__
#define BISON_ANDROID_COMMON_BISON_PATHS_H__

// This file declares path keys for webview. These can be used with
// the PathService to access various special directories and files.

namespace bison {

enum {
  PATH_START = 11000,

  DIR_CRASH_DUMPS = PATH_START,  // Directory where crash dumps are written.

  DIR_SAFE_BROWSING,  // Directory where safe browsing related cookies are
                      // stored.

  PATH_END
};

// Call once to register the provider for the path keys defined above.
void RegisterPathProvider();

}  // namespace bison

#endif  // BISON_ANDROID_COMMON_BISON_PATHS_H__
