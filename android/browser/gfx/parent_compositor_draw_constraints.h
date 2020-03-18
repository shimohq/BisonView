// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_BROWSER_GFX_PARENT_COMPOSITOR_DRAW_CONSTRAINTS_H_
#define BISON_ANDROID_BROWSER_GFX_PARENT_COMPOSITOR_DRAW_CONSTRAINTS_H_

#include "ui/gfx/geometry/size.h"
#include "ui/gfx/transform.h"

namespace bison {

class ChildFrame;

struct ParentCompositorDrawConstraints {
  gfx::Size viewport_size;
  gfx::Transform transform;

  ParentCompositorDrawConstraints();
  ParentCompositorDrawConstraints(const gfx::Size& viewport_size,
                                  const gfx::Transform& transform);
  bool NeedUpdate(const ChildFrame& frame) const;

  bool operator==(const ParentCompositorDrawConstraints& other) const;
};

}  // namespace bison

#endif  // BISON_ANDROID_BROWSER_GFX_PARENT_COMPOSITOR_DRAW_CONSTRAINTS_H_
