// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/browser/gfx/parent_compositor_draw_constraints.h"

#include "bison/android/browser/gfx/child_frame.h"

namespace bison {

ParentCompositorDrawConstraints::ParentCompositorDrawConstraints() = default;

ParentCompositorDrawConstraints::ParentCompositorDrawConstraints(
    const gfx::Size& viewport_size,
    const gfx::Transform& transform)
    : viewport_size(viewport_size), transform(transform) {}

bool ParentCompositorDrawConstraints::NeedUpdate(
    const ChildFrame& frame) const {
  return viewport_size != frame.viewport_size_for_tile_priority ||
         transform != frame.transform_for_tile_priority;
}

bool ParentCompositorDrawConstraints::operator==(
    const ParentCompositorDrawConstraints& other) const {
  return viewport_size == other.viewport_size && transform == other.transform;
}

}  // namespace bison
