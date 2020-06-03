// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_LIB_BISON_MAIN_DELEGATE_H_
#define BISON_CORE_LIB_BISON_MAIN_DELEGATE_H_

#include <memory>

#include "bison/core/browser/bison_feature_list_creator.h"
#include "bison/core/common/bison_content_client.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/public/app/content_main_delegate.h"

namespace content {
class BrowserMainRunner;
}

namespace safe_browsing {
class SafeBrowsingApiHandler;
}

namespace bison {

class BisonContentBrowserClient;
class BisonContentGpuClient;
class BisonContentRendererClient;

// Android WebView implementation of ContentMainDelegate. The methods in
// this class runs per process, (browser and renderer) so when making changes
// make sure to properly conditionalize for browser vs. renderer wherever
// needed.
class BisonMainDelegate : public content::ContentMainDelegate {
 public:
  BisonMainDelegate();
  ~BisonMainDelegate() override;

 private:
  // content::ContentMainDelegate implementation:
  bool BasicStartupComplete(int* exit_code) override;
  void PreSandboxStartup() override;
  int RunProcess(
      const std::string& process_type,
      const content::MainFunctionParams& main_function_params) override;
  void ProcessExiting(const std::string& process_type) override;
  bool ShouldCreateFeatureList() override;
  void PostEarlyInitialization(bool is_running_tests) override;
  void PostFieldTrialInitialization() override;
  content::ContentBrowserClient* CreateContentBrowserClient() override;
  content::ContentGpuClient* CreateContentGpuClient() override;
  content::ContentRendererClient* CreateContentRendererClient() override;

  // Responsible for creating a feature list from the seed. This object must
  // exist for the lifetime of the process as it contains the FieldTrialList
  // that can be queried for the state of experiments.
  std::unique_ptr<BisonFeatureListCreator> aw_feature_list_creator_;
  std::unique_ptr<content::BrowserMainRunner> browser_runner_;
  BisonContentClient content_client_;
  std::unique_ptr<BisonContentBrowserClient> content_browser_client_;
  std::unique_ptr<BisonContentGpuClient> content_gpu_client_;
  std::unique_ptr<BisonContentRendererClient> content_renderer_client_;
  std::unique_ptr<safe_browsing::SafeBrowsingApiHandler>
      safe_browsing_api_handler_;

  DISALLOW_COPY_AND_ASSIGN(BisonMainDelegate);
};

}  // namespace bison

#endif  // BISON_CORE_LIB_BISON_MAIN_DELEGATE_H_
