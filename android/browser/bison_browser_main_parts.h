// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_BROWSER_BISON_BISON_ANDROID_BROWSER_MAIN_PARTS_H_
#define BISON_ANDROID_BROWSER_BISON_BISON_ANDROID_BROWSER_MAIN_PARTS_H_

#include <memory>

#include "bison/android/browser/bison_browser_process.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/task/single_thread_task_executor.h"
#include "content/public/browser/browser_main_parts.h"

namespace bison {

class BisonBrowserProcess;
class BisonContentBrowserClient;
class MemoryMetricsLogger;

class BisonBrowserMainParts : public content::BrowserMainParts {
 public:
  explicit BisonBrowserMainParts(BisonContentBrowserClient* browser_client);
  ~BisonBrowserMainParts() override;

  // Overriding methods from content::BrowserMainParts.
  int PreEarlyInitialization() override;
  int PreCreateThreads() override;
  void PreMainMessageLoopRun() override;
  bool MainMessageLoopRun(int* result_code) override;
  void PostCreateThreads() override;

 private:
  // Android specific UI SingleThreadTaskExecutor.
  std::unique_ptr<base::SingleThreadTaskExecutor> main_task_executor_;

  BisonContentBrowserClient* browser_client_;

  std::unique_ptr<MemoryMetricsLogger> metrics_logger_;

  std::unique_ptr<BisonBrowserProcess> browser_process_;

  DISALLOW_COPY_AND_ASSIGN(BisonBrowserMainParts);
};

}  // namespace bison

#endif  // BISON_ANDROID_BROWSER_BISON_BISON_ANDROID_BROWSER_MAIN_PARTS_H_
