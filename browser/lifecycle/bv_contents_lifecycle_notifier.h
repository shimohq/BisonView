// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_BROWSER_LIFECYCLE_BV_CONTENTS_LIFECYCLE_NOTIFIER_H_
#define BISON_BROWSER_LIFECYCLE_BV_CONTENTS_LIFECYCLE_NOTIFIER_H_

#include <map>

#include "bison/browser/lifecycle/webview_app_state_observer.h"

#include "base/android/jni_android.h"
#include "base/callback.h"
#include "base/callback_forward.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"

namespace bison {

class BvContents;

class BvContentsLifecycleNotifier {
 public:
  using OnLoseForegroundCallback = base::RepeatingClosure;

  enum class BvContentsState {
    // BvContents isn't attached to a window.
    kDetached,
    // BvContents is attached to a window and window is visible.
    kForeground,
    // BvContents is attached to a window and window is invisible.
    kBackground,
  };

  static BvContentsLifecycleNotifier& GetInstance();

  // The |onLoseForegroundCallback| will be invoked after all observers when app
  // lose foreground.
  explicit BvContentsLifecycleNotifier(
      OnLoseForegroundCallback on_lose_foreground_callback);

  BvContentsLifecycleNotifier(const BvContentsLifecycleNotifier&) = delete;
  BvContentsLifecycleNotifier& operator=(const BvContentsLifecycleNotifier&) =
      delete;

  virtual ~BvContentsLifecycleNotifier();

  void OnWebViewCreated(const BvContents* bv_contents);
  void OnWebViewDestroyed(const BvContents* bv_contents);
  void OnWebViewAttachedToWindow(const BvContents* bv_contents);
  void OnWebViewDetachedFromWindow(const BvContents* bv_contents);
  void OnWebViewWindowBeVisible(const BvContents* bv_contents);
  void OnWebViewWindowBeInvisible(const BvContents* bv_contents);

  void AddObserver(WebViewAppStateObserver* observer);
  void RemoveObserver(WebViewAppStateObserver* observer);

  bool has_bv_contents_ever_created() const {
    return has_bv_contents_ever_created_;
  }

  std::vector<const BvContents*> GetAllBvContents() const;

 private:
  struct BvContentsData {
    BvContentsData();
    BvContentsData(BvContentsData&& data);
    BvContentsData(const BvContentsData&) = delete;
    ~BvContentsData();

    bool attached_to_window = false;
    bool window_visible = false;
    BvContentsState bv_content_state = BvContentsState::kDetached;
  };

  friend class TestBvContentsLifecycleNotifier;

  void EnsureOnValidSequence() const {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  }

  size_t ToIndex(BvContentsState state) const;
  void OnBvContentsStateChanged(
      BvContentsLifecycleNotifier::BvContentsData* data);

  void UpdateAppState();

  bool HasBvContentsInstance() const;

  BvContentsLifecycleNotifier::BvContentsData* GetBvContentsData(
      const BvContents* bv_contents);

  // The BvContents to BvContentsData mapping.
  std::map<const BvContents*, BvContentsLifecycleNotifier::BvContentsData>
      bv_contents_to_data_;

  // The number of BvContents instances in each BvContentsState.
  int state_count_[3]{};

  bool has_bv_contents_ever_created_ = false;

  base::ObserverList<WebViewAppStateObserver>::Unchecked observers_;

  OnLoseForegroundCallback on_lose_foreground_callback_;

  WebViewAppStateObserver::State app_state_ =
      WebViewAppStateObserver::State::kDestroyed;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace bison

#endif  // BISON_BROWSER_LIFECYCLE_BV_CONTENTS_LIFECYCLE_NOTIFIER_H_
