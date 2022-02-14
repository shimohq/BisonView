// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_BROWSER_LIFECYCLE_WEBVIEW_APP_STATE_OBSERVER_H_
#define BISON_BROWSER_LIFECYCLE_WEBVIEW_APP_STATE_OBSERVER_H_

namespace bison {

// The interface for being notified of app state change, the implementation
// shall be added to observer list through BvContentsLifecycleNotifier.
class WebViewAppStateObserver {
 public:
  enum class State {
    // All BisonViews are detached from window.
    kUnknown,
    // At least one BisonView is foreground.
    kForeground,
    // No BisonView is foreground and at least one BisonView is background.
    kBackground,
    // All BisonViews are destroyed or no BisonView has been created.
    // Observers shall use
    // BvContentsLifecycleNotifier::has_bv_contents_ever_created() to find if A
    // BisonView has ever been created.
    kDestroyed,
  };

  WebViewAppStateObserver();
  virtual ~WebViewAppStateObserver();

  // Invoked when app state is changed or right after this observer is added
  // into observer list.
  virtual void OnAppStateChanged(State state) = 0;
};

}  // namespace bison

#endif  // BISON_BROWSER_LIFECYCLE_WEBVIEW_APP_STATE_OBSERVER_H_
