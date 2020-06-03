// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_COMMON_CRASH_REPORTER_BISON_CRASH_REPORTER_CLIENT_H_
#define BISON_CORE_COMMON_CRASH_REPORTER_BISON_CRASH_REPORTER_CLIENT_H_

#include <string>

namespace bison {

void EnableCrashReporter(const std::string& process_type);

bool CrashReporterEnabled();

}  // namespace bison

#endif  // BISON_CORE_COMMON_CRASH_REPORTER_BISON_CRASH_REPORTER_CLIENT_H_
