// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Contains constants for known URLs and portions thereof.

#ifndef BISON_ANDROID_COMMON_URL_CONSTANTS_H_
#define BISON_ANDROID_COMMON_URL_CONSTANTS_H_

#include "url/gurl.h"

namespace bison {

// Special Android file paths.
extern const char kAndroidAssetPath[];
extern const char kAndroidResourcePath[];
// Returns whether the given URL is for loading a file from a special path.
bool IsAndroidSpecialFileUrl(const GURL& url);

extern const char kAndroidWebViewVideoPosterScheme[];

}  // namespace bison

#endif  // BISON_ANDROID_COMMON_URL_CONSTANTS_H_
