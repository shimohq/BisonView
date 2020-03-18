// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/browser/gfx/bison_gl_functor.h"

#include "bison/android/browser_jni_headers/BisonGLFunctor_jni.h"
#include "bison/android/public/browser/draw_gl.h"
#include "base/stl_util.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

using base::android::AttachCurrentThread;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;

extern "C" {
static BisonDrawGLFunction DrawGLFunction;
static void DrawGLFunction(long view_context,
                           BisonDrawGLInfo* draw_info,
                           void* spare) {
  // |view_context| is the value that was returned from the java
  // BisonContents.onPrepareDrawGL; this cast must match the code there.
  reinterpret_cast<bison::BisonGLFunctor*>(view_context)
      ->DrawGL(draw_info);
}
}

namespace bison {

namespace {
int g_instance_count = 0;
}

BisonGLFunctor::BisonGLFunctor(const JavaObjectWeakGlobalRef& java_ref)
    : java_ref_(java_ref),
      render_thread_manager_(
          base::CreateSingleThreadTaskRunner({BrowserThread::UI})) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ++g_instance_count;
}

BisonGLFunctor::~BisonGLFunctor() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  --g_instance_count;
}

bool BisonGLFunctor::RequestInvokeGL(bool wait_for_completion) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return false;
  return Java_BisonGLFunctor_requestInvokeGL(env, obj, wait_for_completion);
}

void BisonGLFunctor::DetachFunctorFromView() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (!obj.is_null())
    Java_BisonGLFunctor_detachFunctorFromView(env, obj);
}

void BisonGLFunctor::Destroy(JNIEnv* env) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  java_ref_.reset();
  delete this;
}

void BisonGLFunctor::DeleteHardwareRenderer(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RenderThreadManager::InsideHardwareReleaseReset release_reset(
      &render_thread_manager_);
  DetachFunctorFromView();

  // Receiving at least one frame is a precondition for
  // initialization (such as looing up GL bindings and constructing
  // hardware_renderer_).
  bool draw_functor_succeeded = RequestInvokeGL(true);
  if (!draw_functor_succeeded) {
    LOG(ERROR) << "Unable to free GL resources. Has the Window leaked?";
    // Calling release on wrong thread intentionally.
    render_thread_manager_.DestroyHardwareRendererOnRT(true /* save_restore */);
  }
}

void BisonGLFunctor::DrawGL(BisonDrawGLInfo* draw_info) {
  TRACE_EVENT0("bison,toplevel", "DrawFunctor");
  bool save_restore = draw_info->version < 3;
  switch (draw_info->mode) {
    case BisonDrawGLInfo::kModeSync:
      TRACE_EVENT_INSTANT0("bison", "kModeSync",
                           TRACE_EVENT_SCOPE_THREAD);
      render_thread_manager_.CommitFrameOnRT();
      break;
    case BisonDrawGLInfo::kModeProcessNoContext:
      LOG(ERROR) << "Received unexpected kModeProcessNoContext";
      FALLTHROUGH;
    case BisonDrawGLInfo::kModeProcess:
      render_thread_manager_.DestroyHardwareRendererOnRT(save_restore);
      break;
    case BisonDrawGLInfo::kModeDraw: {
      HardwareRendererDrawParams params{
          draw_info->clip_left,   draw_info->clip_top, draw_info->clip_right,
          draw_info->clip_bottom, draw_info->width,    draw_info->height,
      };
      static_assert(base::size(decltype(draw_info->transform){}) ==
                        base::size(params.transform),
                    "transform size mismatch");
      for (unsigned int i = 0; i < base::size(params.transform); ++i) {
        params.transform[i] = draw_info->transform[i];
      }
      render_thread_manager_.DrawOnRT(save_restore, &params);
      break;
    }
  }
}

void BisonGLFunctor::RemoveFromCompositorFrameProducer(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  render_thread_manager_.RemoveFromCompositorFrameProducerOnUI();
}

jlong BisonGLFunctor::GetCompositorFrameConsumer(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return reinterpret_cast<intptr_t>(GetCompositorFrameConsumer());
}

static jint JNI_BisonGLFunctor_GetNativeInstanceCount(JNIEnv* env) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return g_instance_count;
}

static jlong JNI_BisonGLFunctor_GetBisonDrawGLFunction(JNIEnv* env) {
  return reinterpret_cast<intptr_t>(&DrawGLFunction);
}

static jlong JNI_BisonGLFunctor_Create(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return reinterpret_cast<intptr_t>(
      new BisonGLFunctor(JavaObjectWeakGlobalRef(env, obj)));
}

}  // namespace bison
