// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_COMMON_CRASH_REPORTER_CRASH_KEYS_H_
#define BISON_ANDROID_COMMON_CRASH_REPORTER_CRASH_KEYS_H_

namespace bison {
namespace crash_keys {

// Registers all of the potential crash keys that can be sent to the crash
// reporting server. Returns the size of the union of all keys.
void InitCrashKeysForWebViewTesting();

extern const char* const kWebViewCrashKeyWhiteList[];

// Crash Key Name Constants ////////////////////////////////////////////////////

// Application information.
extern const char kAppPackageName[];
extern const char kAppPackageVersionCode[];

extern const char kAndroidSdkInt[];

}  // namespace crash_keys
}  // namespace bison

#endif  // BISON_ANDROID_COMMON_CRASH_REPORTER_CRASH_KEYS_H_
