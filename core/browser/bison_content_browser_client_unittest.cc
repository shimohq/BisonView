// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/core/browser/bison_content_browser_client.h"

#include "bison/core/browser/bison_feature_list_creator.h"
#include "base/test/task_environment.h"
#include "mojo/core/embedder/embedder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace bison {

class BisonContentBrowserClientTest : public testing::Test {
 protected:
  void SetUp() override {
    mojo::core::Init();
  }

  base::test::TaskEnvironment task_environment_;
};

TEST_F(BisonContentBrowserClientTest, DisableCreatingThreadPool) {
  BisonFeatureListCreator aw_feature_list_creator;
  BisonContentBrowserClient client(&aw_feature_list_creator);
  EXPECT_TRUE(client.ShouldCreateThreadPool());

  BisonContentBrowserClient::DisableCreatingThreadPool();
  EXPECT_FALSE(client.ShouldCreateThreadPool());
}

}  // namespace bison
