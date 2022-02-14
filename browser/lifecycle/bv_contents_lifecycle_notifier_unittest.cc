// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/browser/lifecycle/bv_contents_lifecycle_notifier.h"

#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace bison {

class TestWebViewAppObserver : public WebViewAppStateObserver {
 public:
  TestWebViewAppObserver() = default;
  ~TestWebViewAppObserver() override = default;

  // WebViewAppStateObserver.
  void OnAppStateChanged(State state) override { state_ = state; }

  WebViewAppStateObserver::State state() const { return state_; }

 private:
  WebViewAppStateObserver::State state_;
};

class TestOnLoseForegroundCallback {
 public:
  explicit TestOnLoseForegroundCallback(const TestWebViewAppObserver* other)
      : other_(other) {}

  ~TestOnLoseForegroundCallback() = default;

  void OnLoseForeground() {
    ASSERT_NE(other_->state(), WebViewAppStateObserver::State::kForeground);
    called_ = true;
  }
  bool called() const { return called_; }

 private:
  bool called_ = false;
  const TestWebViewAppObserver* other_;
};

class TestBvContentsLifecycleNotifier : public BvContentsLifecycleNotifier {
 public:
  explicit TestBvContentsLifecycleNotifier(OnLoseForegroundCallback callback)
      : BvContentsLifecycleNotifier(callback) {}
  ~TestBvContentsLifecycleNotifier() override = default;

  size_t GetBvContentsStateCount(BvContentsState state) const {
    return state_count_[ToIndex(state)];
  }

  bool HasBvContentsInstanceForTesting() const {
    return this->HasBvContentsInstance();
  }
};

class BvContentsLifecycleNotifierTest : public testing::Test {
 public:
  WebViewAppStateObserver::State GetState() const { return observer_->state(); }
  size_t GetBvContentsStateCount(
      BvContentsLifecycleNotifier::BvContentsState state) const {
    return notifier_->GetBvContentsStateCount(state);
  }

  bool HasBvContentsInstance() const {
    return notifier_->HasBvContentsInstanceForTesting();
  }

  bool HasBvContentsEverCreated() const {
    return notifier_->has_bv_contents_ever_created();
  }

  BvContentsLifecycleNotifier* notifier() { return notifier_.get(); }

  void VerifyBvContentsStateCount(size_t detached_count,
                                  size_t foreground_count,
                                  size_t background_count) {
    ASSERT_EQ(GetBvContentsStateCount(
                  BvContentsLifecycleNotifier::BvContentsState::kDetached),
              detached_count);
    ASSERT_EQ(GetBvContentsStateCount(
                  BvContentsLifecycleNotifier::BvContentsState::kForeground),
              foreground_count);
    ASSERT_EQ(GetBvContentsStateCount(
                  BvContentsLifecycleNotifier::BvContentsState::kBackground),
              background_count);
  }

  const TestWebViewAppObserver* observer() const { return observer_.get(); }
  const TestOnLoseForegroundCallback* callback() const {
    return callback_.get();
  }

 protected:
  // testing::Test.
  void SetUp() override {
    observer_ = std::make_unique<TestWebViewAppObserver>();
    callback_ = std::make_unique<TestOnLoseForegroundCallback>(observer_.get());
    notifier_ = std::make_unique<TestBvContentsLifecycleNotifier>(
        base::BindRepeating(&TestOnLoseForegroundCallback::OnLoseForeground,
                            base::Unretained(callback_.get())));

    notifier_->AddObserver(observer_.get());
  }

  void TearDown() override { notifier_->RemoveObserver(observer_.get()); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestWebViewAppObserver> observer_;
  std::unique_ptr<TestOnLoseForegroundCallback> callback_;
  std::unique_ptr<TestBvContentsLifecycleNotifier> notifier_;
};

TEST_F(BvContentsLifecycleNotifierTest, Created) {
  const BvContents* fake_bv_contents = reinterpret_cast<const BvContents*>(1);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kDestroyed);
  ASSERT_FALSE(HasBvContentsEverCreated());
  ASSERT_FALSE(HasBvContentsInstance());

  notifier()->OnWebViewCreated(fake_bv_contents);
  VerifyBvContentsStateCount(1u, 0, 0);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kUnknown);
  ASSERT_TRUE(HasBvContentsInstance());
  ASSERT_TRUE(HasBvContentsEverCreated());

  notifier()->OnWebViewDestroyed(fake_bv_contents);
  VerifyBvContentsStateCount(0, 0, 0);
  ASSERT_FALSE(HasBvContentsInstance());
  ASSERT_TRUE(HasBvContentsEverCreated());
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kDestroyed);
}

TEST_F(BvContentsLifecycleNotifierTest, AttachToAndDetachFromWindow) {
  const BvContents* fake_bv_contents = reinterpret_cast<const BvContents*>(1);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kDestroyed);
  ASSERT_FALSE(HasBvContentsEverCreated());
  ASSERT_FALSE(HasBvContentsInstance());

  notifier()->OnWebViewCreated(fake_bv_contents);
  notifier()->OnWebViewAttachedToWindow(fake_bv_contents);
  VerifyBvContentsStateCount(0, 0, 1u);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kBackground);
  ASSERT_TRUE(HasBvContentsInstance());
  ASSERT_TRUE(HasBvContentsEverCreated());

  notifier()->OnWebViewDetachedFromWindow(fake_bv_contents);
  VerifyBvContentsStateCount(1u, 0, 0);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kUnknown);
  ASSERT_TRUE(HasBvContentsInstance());
  ASSERT_TRUE(HasBvContentsEverCreated());

  notifier()->OnWebViewDestroyed(fake_bv_contents);
  VerifyBvContentsStateCount(0, 0, 0);
  ASSERT_FALSE(HasBvContentsInstance());
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kDestroyed);
}

TEST_F(BvContentsLifecycleNotifierTest, WindowVisibleAndInvisible) {
  const BvContents* fake_bv_contents = reinterpret_cast<const BvContents*>(1);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kDestroyed);
  ASSERT_FALSE(HasBvContentsEverCreated());

  notifier()->OnWebViewCreated(fake_bv_contents);
  notifier()->OnWebViewAttachedToWindow(fake_bv_contents);
  notifier()->OnWebViewWindowBeVisible(fake_bv_contents);
  VerifyBvContentsStateCount(0, 1u, 0);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kForeground);
  ASSERT_TRUE(HasBvContentsEverCreated());

  notifier()->OnWebViewWindowBeInvisible(fake_bv_contents);
  VerifyBvContentsStateCount(0, 0, 1u);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kBackground);

  notifier()->OnWebViewDetachedFromWindow(fake_bv_contents);
  VerifyBvContentsStateCount(1u, 0, 0);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kUnknown);

  notifier()->OnWebViewDestroyed(fake_bv_contents);
  VerifyBvContentsStateCount(0, 0, 0);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kDestroyed);
  ASSERT_TRUE(HasBvContentsEverCreated());
}

TEST_F(BvContentsLifecycleNotifierTest, MultipleBvContents) {
  const BvContents* fake_bv_contents1 = reinterpret_cast<const BvContents*>(1);
  const BvContents* fake_bv_contents2 = reinterpret_cast<const BvContents*>(2);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kDestroyed);
  ASSERT_FALSE(HasBvContentsEverCreated());

  notifier()->OnWebViewCreated(fake_bv_contents1);
  VerifyBvContentsStateCount(1u, 0, 0);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kUnknown);
  ASSERT_TRUE(HasBvContentsEverCreated());

  notifier()->OnWebViewAttachedToWindow(fake_bv_contents1);
  VerifyBvContentsStateCount(0, 0, 1u);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kBackground);

  notifier()->OnWebViewCreated(fake_bv_contents2);
  VerifyBvContentsStateCount(1u, 0, 1u);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kBackground);

  notifier()->OnWebViewAttachedToWindow(fake_bv_contents2);
  VerifyBvContentsStateCount(0, 0, 2u);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kBackground);

  notifier()->OnWebViewWindowBeVisible(fake_bv_contents2);
  VerifyBvContentsStateCount(0, 1u, 1u);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kForeground);

  notifier()->OnWebViewWindowBeVisible(fake_bv_contents1);
  VerifyBvContentsStateCount(0, 2u, 0);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kForeground);

  notifier()->OnWebViewDestroyed(fake_bv_contents2);
  VerifyBvContentsStateCount(0, 1u, 0);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kForeground);

  notifier()->OnWebViewWindowBeInvisible(fake_bv_contents1);
  VerifyBvContentsStateCount(0, 0, 1u);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kBackground);

  notifier()->OnWebViewDetachedFromWindow(fake_bv_contents1);
  VerifyBvContentsStateCount(1u, 0, 0);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kUnknown);

  notifier()->OnWebViewDestroyed(fake_bv_contents1);
  VerifyBvContentsStateCount(0, 0, 0);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kDestroyed);

  notifier()->OnWebViewCreated(fake_bv_contents1);
  VerifyBvContentsStateCount(1u, 0, 0);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kUnknown);
  ASSERT_TRUE(HasBvContentsEverCreated());
}

TEST_F(BvContentsLifecycleNotifierTest, AttachedToWindowAfterWindowVisible) {
  const BvContents* fake_bv_contents = reinterpret_cast<const BvContents*>(1);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kDestroyed);
  ASSERT_FALSE(HasBvContentsEverCreated());

  notifier()->OnWebViewCreated(fake_bv_contents);
  VerifyBvContentsStateCount(1u, 0, 0);
  notifier()->OnWebViewWindowBeVisible(fake_bv_contents);
  VerifyBvContentsStateCount(1u, 0, 0);
  notifier()->OnWebViewAttachedToWindow(fake_bv_contents);
  VerifyBvContentsStateCount(0, 1u, 0);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kForeground);
  ASSERT_TRUE(HasBvContentsEverCreated());
}

TEST_F(BvContentsLifecycleNotifierTest, AttachedToWindowAfterWindowInvisible) {
  const BvContents* fake_bv_contents = reinterpret_cast<const BvContents*>(1);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kDestroyed);
  ASSERT_FALSE(HasBvContentsEverCreated());

  notifier()->OnWebViewCreated(fake_bv_contents);
  VerifyBvContentsStateCount(1u, 0, 0);
  notifier()->OnWebViewWindowBeInvisible(fake_bv_contents);
  VerifyBvContentsStateCount(1u, 0, 0);
  notifier()->OnWebViewAttachedToWindow(fake_bv_contents);
  VerifyBvContentsStateCount(0, 0, 1u);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kBackground);
  ASSERT_TRUE(HasBvContentsEverCreated());
}

TEST_F(BvContentsLifecycleNotifierTest, DetachFromVisibleWindow) {
  const BvContents* fake_bv_contents = reinterpret_cast<const BvContents*>(1);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kDestroyed);
  ASSERT_FALSE(HasBvContentsEverCreated());

  notifier()->OnWebViewCreated(fake_bv_contents);
  VerifyBvContentsStateCount(1u, 0, 0);
  notifier()->OnWebViewWindowBeVisible(fake_bv_contents);
  VerifyBvContentsStateCount(1u, 0, 0);
  notifier()->OnWebViewAttachedToWindow(fake_bv_contents);
  VerifyBvContentsStateCount(0, 1u, 0);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kForeground);
  notifier()->OnWebViewDetachedFromWindow(fake_bv_contents);
  ASSERT_EQ(GetState(), WebViewAppStateObserver::State::kUnknown);
  ASSERT_TRUE(HasBvContentsEverCreated());
}

TEST_F(BvContentsLifecycleNotifierTest, GetAllBvContents) {
  std::vector<const BvContents*> all_bv_contents(
      notifier()->GetAllBvContents());
  ASSERT_TRUE(all_bv_contents.empty());
  const BvContents* fake_bv_contents = reinterpret_cast<const BvContents*>(1);
  notifier()->OnWebViewCreated(fake_bv_contents);
  all_bv_contents = notifier()->GetAllBvContents();
  ASSERT_EQ(all_bv_contents.size(), 1u);
  ASSERT_EQ(all_bv_contents.back(), fake_bv_contents);
  const BvContents* fake_bv_contents2 = reinterpret_cast<const BvContents*>(2);
  notifier()->OnWebViewCreated(fake_bv_contents2);
  all_bv_contents = notifier()->GetAllBvContents();
  ASSERT_EQ(all_bv_contents.size(), 2u);
  ASSERT_EQ(all_bv_contents.front(), fake_bv_contents);
  ASSERT_EQ(all_bv_contents.back(), fake_bv_contents2);
  notifier()->OnWebViewDestroyed(fake_bv_contents);
  all_bv_contents = notifier()->GetAllBvContents();
  ASSERT_EQ(all_bv_contents.size(), 1u);
  ASSERT_EQ(all_bv_contents.back(), fake_bv_contents2);
  notifier()->OnWebViewDestroyed(fake_bv_contents2);
  all_bv_contents = notifier()->GetAllBvContents();
  ASSERT_TRUE(all_bv_contents.empty());
}

TEST_F(BvContentsLifecycleNotifierTest, LoseForegroundCallback) {
  const BvContents* fake_bv_contents = reinterpret_cast<const BvContents*>(1);
  notifier()->OnWebViewCreated(fake_bv_contents);
  notifier()->OnWebViewAttachedToWindow(fake_bv_contents);
  notifier()->OnWebViewWindowBeVisible(fake_bv_contents);
  notifier()->OnWebViewWindowBeInvisible(fake_bv_contents);
  EXPECT_TRUE(callback()->called());
}

}  // bison
