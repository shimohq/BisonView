// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/browser/bv_content_browser_client.h"

#include "bison/browser/bv_feature_list_creator.h"
#include "base/test/task_environment.h"
#include "mojo/core/embedder/embedder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace bison {

class BvContentBrowserClientTest : public testing::Test {
 protected:
  void SetUp() override {
    mojo::core::Init();
  }

  base::test::TaskEnvironment task_environment_;
};

TEST_F(BvContentBrowserClientTest, DisableCreatingThreadPool) {
  BvFeatureListCreator bv_feature_list_creator;
  BvContentBrowserClient client(&bv_feature_list_creator);
  EXPECT_TRUE(client.ShouldCreateThreadPool());

  BvContentBrowserClient::DisableCreatingThreadPool();
  EXPECT_FALSE(client.ShouldCreateThreadPool());
}

}  // namespace bison
