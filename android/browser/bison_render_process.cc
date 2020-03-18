// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/browser/bison_render_process.h"

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"

#include "bison/android/browser_jni_headers/BisonRenderProcess_jni.h"

using base::android::AttachCurrentThread;
using content::BrowserThread;
using content::ChildProcessTerminationInfo;
using content::RenderProcessHost;

namespace bison {

const void* const kBisonRenderProcessKey = &kBisonRenderProcessKey;

// static
BisonRenderProcess* BisonRenderProcess::GetInstanceForRenderProcessHost(
    RenderProcessHost* host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  BisonRenderProcess* render_process =
      static_cast<BisonRenderProcess*>(host->GetUserData(kBisonRenderProcessKey));
  if (!render_process) {
    std::unique_ptr<BisonRenderProcess> created_render_process =
        std::make_unique<BisonRenderProcess>(host);
    render_process = created_render_process.get();
    host->SetUserData(kBisonRenderProcessKey, std::move(created_render_process));
  }
  return render_process;
}

BisonRenderProcess::BisonRenderProcess(RenderProcessHost* render_process_host)
    : render_process_host_(render_process_host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  java_obj_.Reset(Java_BisonRenderProcess_create(AttachCurrentThread()));
  CHECK(!java_obj_.is_null());
  if (render_process_host_->IsReady()) {
    Ready();
  }
  render_process_host->AddObserver(this);
}

BisonRenderProcess::~BisonRenderProcess() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Java_BisonRenderProcess_setNative(AttachCurrentThread(), java_obj_, 0);
  java_obj_.Reset();
}

void BisonRenderProcess::Ready() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Java_BisonRenderProcess_setNative(AttachCurrentThread(), java_obj_,
                                 reinterpret_cast<jlong>(this));
}

void BisonRenderProcess::Cleanup() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  render_process_host_->RemoveObserver(this);
  render_process_host_->RemoveUserData(kBisonRenderProcessKey);
  // |this| is now deleted.
}

bool BisonRenderProcess::TerminateChildProcess(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  return render_process_host_->Shutdown(0);
}

base::android::ScopedJavaLocalRef<jobject> BisonRenderProcess::GetJavaObject() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  return base::android::ScopedJavaLocalRef<jobject>(java_obj_);
}

void BisonRenderProcess::RenderProcessReady(RenderProcessHost* host) {
  DCHECK(host == render_process_host_);

  Ready();
}

void BisonRenderProcess::RenderProcessExited(
    RenderProcessHost* host,
    const ChildProcessTerminationInfo& info) {
  DCHECK(host == render_process_host_);

  Cleanup();
}

}  // namespace bison
