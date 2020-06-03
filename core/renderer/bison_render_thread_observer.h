// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_RENDERER_BISON_RENDER_THREAD_OBSERVER_H_
#define BISON_CORE_RENDERER_BISON_RENDER_THREAD_OBSERVER_H_

#include "content/public/renderer/render_thread_observer.h"

#include "base/compiler_specific.h"

namespace bison {

// A RenderThreadObserver implementation used for handling android_webview
// specific render-process wide IPC messages.
class BisonRenderThreadObserver : public content::RenderThreadObserver {
 public:
  BisonRenderThreadObserver();
  ~BisonRenderThreadObserver() override;

  // content::RenderThreadObserver implementation.
  bool OnControlMessageReceived(const IPC::Message& message) override;

 private:
  void OnClearCache();
  void OnKillProcess();
  void OnSetJsOnlineProperty(bool network_up);
};

}  // namespace bison

#endif  // BISON_CORE_RENDERER_BISON_RENDER_THREAD_OBSERVER_H_

