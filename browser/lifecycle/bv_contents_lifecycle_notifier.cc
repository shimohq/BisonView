// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/browser/lifecycle/bv_contents_lifecycle_notifier.h"

#include <utility>

#include "bison/bison_jni_headers/BvContentsLifecycleNotifier_jni.h"
#include "content/public/browser/browser_thread.h"

using base::android::AttachCurrentThread;
using content::BrowserThread;

namespace bison {

namespace {

BvContentsLifecycleNotifier::BvContentsState CalculateState(
    bool is_attached_to_window,
    bool is_window_visible) {
  // Can't assume the sequence of Attached, Detached, Visible, Invisible event
  // because the app could changed it; Calculate the state here.
  if (is_attached_to_window) {
    return is_window_visible
               ? BvContentsLifecycleNotifier::BvContentsState::kForeground
               : BvContentsLifecycleNotifier::BvContentsState::kBackground;
  }
  return BvContentsLifecycleNotifier::BvContentsState::kDetached;
}

BvContentsLifecycleNotifier* g_instance = nullptr;

}  // namespace

BvContentsLifecycleNotifier::BvContentsData::BvContentsData() = default;

BvContentsLifecycleNotifier::BvContentsData::BvContentsData(
    BvContentsData&& data) = default;

BvContentsLifecycleNotifier::BvContentsData::~BvContentsData() = default;

// static
BvContentsLifecycleNotifier& BvContentsLifecycleNotifier::GetInstance() {
  DCHECK(g_instance);
  g_instance->EnsureOnValidSequence();
  return *g_instance;
}

BvContentsLifecycleNotifier::BvContentsLifecycleNotifier(
    OnLoseForegroundCallback on_lose_foreground_callback)
    : on_lose_foreground_callback_(std::move(on_lose_foreground_callback)) {
  EnsureOnValidSequence();
  DCHECK(!g_instance);
  g_instance = this;
}

BvContentsLifecycleNotifier::~BvContentsLifecycleNotifier() {
  EnsureOnValidSequence();
  DCHECK(g_instance);
  g_instance = nullptr;
}

void BvContentsLifecycleNotifier::OnWebViewCreated(
    const BvContents* bv_contents) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  has_bv_contents_ever_created_ = true;
  bool first_created = !HasBvContentsInstance();
  DCHECK(bv_contents_to_data_.find(bv_contents) == bv_contents_to_data_.end());

  bv_contents_to_data_.emplace(bv_contents, BvContentsData());
  state_count_[ToIndex(BvContentsState::kDetached)]++;
  UpdateAppState();

  if (first_created) {
    Java_BvContentsLifecycleNotifier_onFirstWebViewCreated(
        AttachCurrentThread());
  }
}

void BvContentsLifecycleNotifier::OnWebViewDestroyed(
    const BvContents* bv_contents) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  const auto it = bv_contents_to_data_.find(bv_contents);
  DCHECK(it != bv_contents_to_data_.end());

  state_count_[ToIndex(it->second.bv_content_state)]--;
  DCHECK(state_count_[ToIndex(it->second.bv_content_state)] >= 0);
  bv_contents_to_data_.erase(it);
  UpdateAppState();

  if (!HasBvContentsInstance()) {
    Java_BvContentsLifecycleNotifier_onLastWebViewDestroyed(
        AttachCurrentThread());
  }
}

void BvContentsLifecycleNotifier::OnWebViewAttachedToWindow(
    const BvContents* bv_contents) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* data = GetBvContentsData(bv_contents);
  data->attached_to_window = true;
  OnBvContentsStateChanged(data);
}

void BvContentsLifecycleNotifier::OnWebViewDetachedFromWindow(
    const BvContents* bv_contents) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* data = GetBvContentsData(bv_contents);
  data->attached_to_window = false;
  DCHECK(data->bv_content_state != BvContentsState::kDetached);
  OnBvContentsStateChanged(data);
}

void BvContentsLifecycleNotifier::OnWebViewWindowBeVisible(
    const BvContents* bv_contents) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* data = GetBvContentsData(bv_contents);
  data->window_visible = true;
  OnBvContentsStateChanged(data);
}

void BvContentsLifecycleNotifier::OnWebViewWindowBeInvisible(
    const BvContents* bv_contents) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* data = GetBvContentsData(bv_contents);
  data->window_visible = false;
  OnBvContentsStateChanged(data);
}

void BvContentsLifecycleNotifier::AddObserver(
    WebViewAppStateObserver* observer) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  observers_.AddObserver(observer);
  observer->OnAppStateChanged(app_state_);
}

void BvContentsLifecycleNotifier::RemoveObserver(
    WebViewAppStateObserver* observer) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  observers_.RemoveObserver(observer);
}

std::vector<const BvContents*> BvContentsLifecycleNotifier::GetAllBvContents()
    const {
  std::vector<const BvContents*> result;
  result.reserve(bv_contents_to_data_.size());
  for (auto& it : bv_contents_to_data_)
    result.push_back(it.first);
  return result;
}

size_t BvContentsLifecycleNotifier::ToIndex(BvContentsState state) const {
  size_t index = static_cast<size_t>(state);
  DCHECK(index < std::size(state_count_));
  return index;
}

void BvContentsLifecycleNotifier::OnBvContentsStateChanged(
    BvContentsLifecycleNotifier::BvContentsData* data) {
  BvContentsLifecycleNotifier::BvContentsState state =
      CalculateState(data->attached_to_window, data->window_visible);
  if (data->bv_content_state == state)
    return;
  state_count_[ToIndex(data->bv_content_state)]--;
  DCHECK(state_count_[ToIndex(data->bv_content_state)] >= 0);
  state_count_[ToIndex(state)]++;
  data->bv_content_state = state;
  UpdateAppState();
}

void BvContentsLifecycleNotifier::UpdateAppState() {
  WebViewAppStateObserver::State state;
  if (state_count_[ToIndex(BvContentsState::kForeground)] > 0)
    state = WebViewAppStateObserver::State::kForeground;
  else if (state_count_[ToIndex(BvContentsState::kBackground)] > 0)
    state = WebViewAppStateObserver::State::kBackground;
  else if (state_count_[ToIndex(BvContentsState::kDetached)] > 0)
    state = WebViewAppStateObserver::State::kUnknown;
  else
    state = WebViewAppStateObserver::State::kDestroyed;
  if (state != app_state_) {
    bool previous_in_foreground =
        app_state_ == WebViewAppStateObserver::State::kForeground;

    app_state_ = state;
    for (auto& observer : observers_) {
      observer.OnAppStateChanged(app_state_);
    }
    if (previous_in_foreground && on_lose_foreground_callback_) {
      on_lose_foreground_callback_.Run();
    }
  }
}

bool BvContentsLifecycleNotifier::HasBvContentsInstance() const {
  for (size_t i = 0; i < std::size(state_count_); i++) {
    if (state_count_[i] > 0)
      return true;
  }
  return false;
}

BvContentsLifecycleNotifier::BvContentsData*
BvContentsLifecycleNotifier::GetBvContentsData(const BvContents* bv_contents) {
  DCHECK(bv_contents_to_data_.find(bv_contents) != bv_contents_to_data_.end());
  return &bv_contents_to_data_.at(bv_contents);
}

}  // namespace bison
