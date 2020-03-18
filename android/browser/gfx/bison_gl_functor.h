// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_BROWSER_GFX_BISON_GL_FUNCTOR_H_
#define BISON_ANDROID_BROWSER_GFX_BISON_GL_FUNCTOR_H_

#include "bison/android/browser/gfx/compositor_frame_consumer.h"
#include "bison/android/browser/gfx/render_thread_manager.h"
#include "base/android/jni_weak_ref.h"

struct BisonDrawGLInfo;

namespace bison {

class BisonGLFunctor {
 public:
  explicit BisonGLFunctor(const JavaObjectWeakGlobalRef& java_ref);
  ~BisonGLFunctor();

  void Destroy(JNIEnv* env);
  void DeleteHardwareRenderer(JNIEnv* env,
                              const base::android::JavaParamRef<jobject>& obj);
  void RemoveFromCompositorFrameProducer(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  jlong GetCompositorFrameConsumer(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  jlong GetBisonDrawGLFunction(JNIEnv* env,
                            const base::android::JavaParamRef<jobject>& obj);

  void DrawGL(BisonDrawGLInfo* draw_info);

 private:
  bool RequestInvokeGL(bool wait_for_completion);
  void DetachFunctorFromView();
  CompositorFrameConsumer* GetCompositorFrameConsumer() {
    return &render_thread_manager_;
  }

  JavaObjectWeakGlobalRef java_ref_;
  RenderThreadManager render_thread_manager_;
};

}  // namespace bison

#endif  // BISON_ANDROID_BROWSER_GFX_BISON_GL_FUNCTOR_H_
