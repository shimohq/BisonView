// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/browser/bv_browser_context.h"
#include "bison/browser/bv_browser_process.h"
#include "bison/browser/bv_feature_list_creator.h"
#include "bison/browser/network_service/bv_network_change_notifier_factory.h"
#include "base/run_loop.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_content_client_initializer.h"
#include "mojo/core/embedder/embedder.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace bison {

class BvBrowserContextTest : public testing::Test {
 protected:
  // Runs before the first test
  static void SetUpTestSuite() {
    net::NetworkChangeNotifier::SetFactory(
        new BvNetworkChangeNotifierFactory());
  }

  void SetUp() override {
    mojo::core::Init();
    test_content_client_initializer_ =
        new content::TestContentClientInitializer();

    BvFeatureListCreator* bv_feature_list_creator = new BvFeatureListCreator();
    bv_feature_list_creator->CreateLocalState();
    browser_process_ = new BvBrowserProcess(bv_feature_list_creator);
  }

  void TearDown() override {
    // Drain the message queue before destroying
    // |test_content_client_initializer_|, otherwise a posted task may call
    // content::GetNetworkConnectionTracker() after
    // TestContentClientInitializer's destructor sets it to null.
    base::RunLoop().RunUntilIdle();
    delete test_content_client_initializer_;
    delete browser_process_;
  }

  // Create the TestBrowserThreads.
  content::BrowserTaskEnvironment task_environment_;
  content::TestContentClientInitializer* test_content_client_initializer_;
  BvBrowserProcess* browser_process_;
};

// Tests that constraints on trust for Symantec-issued certificates are not
// enforced for the NetworkContext, as it should behave like the Android system.
TEST_F(BvBrowserContextTest, SymantecPoliciesExempted) {
  BvBrowserContext context;
  network::mojom::NetworkContextParams network_context_params;
  network::mojom::CertVerifierCreationParams cert_verifier_params;
  context.ConfigureNetworkContextParams(
      false, base::FilePath(), &network_context_params, &cert_verifier_params);

  ASSERT_TRUE(network_context_params.initial_ssl_config);
  ASSERT_TRUE(
      network_context_params.initial_ssl_config->symantec_enforcement_disabled);
}

// Tests that SHA-1 is still allowed for locally-installed trust anchors,
// including those in application manifests, as it should behave like
// the Android system.
TEST_F(BvBrowserContextTest, SHA1LocalAnchorsAllowed) {
  BvBrowserContext context;
  network::mojom::NetworkContextParams network_context_params;
  network::mojom::CertVerifierCreationParams cert_verifier_params;
  context.ConfigureNetworkContextParams(
      false, base::FilePath(), &network_context_params, &cert_verifier_params);

  ASSERT_TRUE(network_context_params.initial_ssl_config);
  ASSERT_TRUE(
      network_context_params.initial_ssl_config->sha1_local_anchors_enabled);
}

}  // namespace bison_view
