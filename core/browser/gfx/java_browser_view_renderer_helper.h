// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_BROWSER_GFX_JAVA_BROWSER_VIEW_RENDERER_HELPER_H_
#define BISON_CORE_BROWSER_GFX_JAVA_BROWSER_VIEW_RENDERER_HELPER_H_

#include <jni.h>

#include <memory>

#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/vector2d.h"

class SkCanvas;
struct BisonDrawSWFunctionTable;

namespace bison {

class SoftwareCanvasHolder {
 public:
  static std::unique_ptr<SoftwareCanvasHolder> Create(
      jobject java_canvas,
      const gfx::Vector2d& scroll_correction,
      const gfx::Size& auxiliary_bitmap_size,
      bool force_auxiliary_bitmap);

  virtual ~SoftwareCanvasHolder() {}

  // The returned SkCanvas is still owned by this holder.
  virtual SkCanvas* GetCanvas() = 0;
};

void RasterHelperSetBisonDrawSWFunctionTable(BisonDrawSWFunctionTable* table);

}  // namespace bison

#endif  // BISON_CORE_BROWSER_GFX_JAVA_BROWSER_VIEW_RENDERER_HELPER_H_
