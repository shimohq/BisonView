// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/core/browser/gfx/bison_render_thread_context_provider.h"

#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/trace_event/trace_event.h"
#include "components/viz/common/gpu/context_cache_controller.h"
#include "gpu/command_buffer/client/gles2_implementation.h"
#include "gpu/command_buffer/client/gles2_trace_implementation.h"
#include "gpu/command_buffer/client/gpu_switches.h"
#include "gpu/command_buffer/client/shared_memory_limits.h"
#include "gpu/config/gpu_feature_info.h"
#include "gpu/ipc/gl_in_process_context.h"
#include "gpu/skia_bindings/gl_bindings_skia_cmd_buffer.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "third_party/skia/include/gpu/gl/GrGLInterface.h"

namespace bison {

// static
scoped_refptr<BisonRenderThreadContextProvider>
BisonRenderThreadContextProvider::Create(
    scoped_refptr<gl::GLSurface> surface,
    gpu::CommandBufferTaskExecutor* task_executor) {
  return new BisonRenderThreadContextProvider(surface, task_executor);
}

BisonRenderThreadContextProvider::BisonRenderThreadContextProvider(
    scoped_refptr<gl::GLSurface> surface,
    gpu::CommandBufferTaskExecutor* task_executor) {
  DCHECK(main_thread_checker_.CalledOnValidThread());

  // This is an onscreen context, wrapping the GLSurface given to us from
  // the Android OS. The widget we pass here will be ignored since we're
  // providing the GLSurface to the context already.
  DCHECK(!surface->IsOffscreen());
  gpu::ContextCreationAttribs attributes;
  // The context is wrapping an already allocated surface, so we can't control
  // what buffers it has from these attributes. We do expect an alpha and
  // stencil buffer to exist for webview, as the display compositor requires
  // having them both in order to integrate its output with the content behind
  // it.
  attributes.alpha_size = 8;
  attributes.stencil_size = 8;
  // The depth buffer may exist due to having a stencil buffer, but we don't
  // need one, so use -1 for it.
  attributes.depth_size = -1;
  attributes.samples = 0;
  attributes.sample_buffers = 0;
  attributes.bind_generates_resource = false;

  gpu::SharedMemoryLimits limits;
  // This context is only used for the display compositor, and there are no
  // uploads done with it at all. We choose a small transfer buffer limit
  // here, the minimums match the display compositor context for the android
  // browser. We don't set the max since we expect the transfer buffer to be
  // relatively unused.
  limits.start_transfer_buffer_size = 64 * 1024;
  limits.min_transfer_buffer_size = 64 * 1024;

  context_ = std::make_unique<gpu::GLInProcessContext>();
  context_->Initialize(task_executor, surface, surface->IsOffscreen(),
                       gpu::kNullSurfaceHandle, attributes, limits, nullptr,
                       nullptr, nullptr);

  context_->GetImplementation()->SetLostContextCallback(base::BindOnce(
      &BisonRenderThreadContextProvider::OnLostContext, base::Unretained(this)));

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableGpuClientTracing)) {
    // This wraps the real GLES2Implementation and we should always use this
    // instead when it's present.
    trace_impl_ = std::make_unique<gpu::gles2::GLES2TraceImplementation>(
        context_->GetImplementation());
  }

  cache_controller_ = std::make_unique<viz::ContextCacheController>(
      context_->GetImplementation(), nullptr);
}

BisonRenderThreadContextProvider::~BisonRenderThreadContextProvider() {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  if (gr_context_)
    gr_context_->releaseResourcesAndAbandonContext();
}

uint32_t BisonRenderThreadContextProvider::GetCopyTextureInternalFormat() {
  // The attributes used in the constructor included an alpha channel.
  return GL_RGBA;
}

void BisonRenderThreadContextProvider::AddRef() const {
  base::RefCountedThreadSafe<BisonRenderThreadContextProvider>::AddRef();
}

void BisonRenderThreadContextProvider::Release() const {
  base::RefCountedThreadSafe<BisonRenderThreadContextProvider>::Release();
}

gpu::ContextResult BisonRenderThreadContextProvider::BindToCurrentThread() {
  // This is called on the thread the context will be used.
  DCHECK(main_thread_checker_.CalledOnValidThread());

  return gpu::ContextResult::kSuccess;
}

const gpu::Capabilities& BisonRenderThreadContextProvider::ContextCapabilities()
    const {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  return context_->GetImplementation()->capabilities();
}

const gpu::GpuFeatureInfo& BisonRenderThreadContextProvider::GetGpuFeatureInfo()
    const {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  return context_->GetGpuFeatureInfo();
}

gpu::gles2::GLES2Interface* BisonRenderThreadContextProvider::ContextGL() {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  if (trace_impl_)
    return trace_impl_.get();
  return context_->GetImplementation();
}

gpu::ContextSupport* BisonRenderThreadContextProvider::ContextSupport() {
  DCHECK(main_thread_checker_.CalledOnValidThread());

  return context_->GetImplementation();
}

class GrContext* BisonRenderThreadContextProvider::GrContext() {
  DCHECK(main_thread_checker_.CalledOnValidThread());

  if (gr_context_)
    return gr_context_.get();

  sk_sp<GrGLInterface> interface(skia_bindings::CreateGLES2InterfaceBindings(
      ContextGL(), ContextSupport()));
  gr_context_ = GrContext::MakeGL(std::move(interface));
  cache_controller_->SetGrContext(gr_context_.get());
  return gr_context_.get();
}

gpu::SharedImageInterface*
BisonRenderThreadContextProvider::SharedImageInterface() {
  return context_->GetSharedImageInterface();
}

viz::ContextCacheController* BisonRenderThreadContextProvider::CacheController() {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  return cache_controller_.get();
}

base::Lock* BisonRenderThreadContextProvider::GetLock() {
  // This context provider is not used on multiple threads.
  NOTREACHED();
  return nullptr;
}

void BisonRenderThreadContextProvider::AddObserver(viz::ContextLostObserver* obs) {
  observers_.AddObserver(obs);
}

void BisonRenderThreadContextProvider::RemoveObserver(
    viz::ContextLostObserver* obs) {
  observers_.RemoveObserver(obs);
}

void BisonRenderThreadContextProvider::OnLostContext() {
  DCHECK(main_thread_checker_.CalledOnValidThread());

  for (auto& observer : observers_)
    observer.OnContextLost();
  if (gr_context_)
    gr_context_->abandonContext();
}

}  // namespace bison