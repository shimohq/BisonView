// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/core/browser/gfx/bison_gl_surface.h"

#include <utility>

#include "bison/core/browser/gfx/scoped_app_gl_state_restore.h"

namespace bison {

BisonGLSurface::BisonGLSurface() : size_(1, 1) {}

BisonGLSurface::~BisonGLSurface() {}

void BisonGLSurface::Destroy() {
}

bool BisonGLSurface::IsOffscreen() {
  return false;
}

unsigned int BisonGLSurface::GetBackingFramebufferObject() {
  return ScopedAppGLStateRestore::Current()->framebuffer_binding_ext();
}

gfx::SwapResult BisonGLSurface::SwapBuffers(PresentationCallback callback) {
  DCHECK(pending_presentation_callback_.is_null());
  pending_presentation_callback_ = std::move(callback);
  return gfx::SwapResult::SWAP_ACK;
}

gfx::Size BisonGLSurface::GetSize() {
  return size_;
}

void* BisonGLSurface::GetHandle() {
  return NULL;
}

void* BisonGLSurface::GetDisplay() {
  return NULL;
}

gl::GLSurfaceFormat BisonGLSurface::GetFormat() {
  return gl::GLSurfaceFormat();
}

bool BisonGLSurface::Resize(const gfx::Size& size,
                         float scale_factor,
                         ColorSpace color_space,
                         bool has_alpha) {
  size_ = size;
  return true;
}

void BisonGLSurface::SetSize(const gfx::Size& size) {
  size_ = size;
}

void BisonGLSurface::MaybeDidPresent(gfx::PresentationFeedback feedback) {
  if (pending_presentation_callback_.is_null())
    return;
  std::move(pending_presentation_callback_).Run(std::move(feedback));
}

}  // namespace bison
