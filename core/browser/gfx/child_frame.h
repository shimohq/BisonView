// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_BROWSER_GFX_CHILD_FRAME_H_
#define BISON_CORE_BROWSER_GFX_CHILD_FRAME_H_

#include <memory>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "content/public/browser/android/synchronous_compositor.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/transform.h"

namespace viz {
class CompositorFrame;
class CopyOutputRequest;
}

namespace bison {

using CopyOutputRequestQueue =
    std::vector<std::unique_ptr<viz::CopyOutputRequest>>;

class ChildFrame {
 public:
  ChildFrame(
      scoped_refptr<content::SynchronousCompositor::FrameFuture> frame_future,
      const viz::FrameSinkId& frame_sink_id,
      const gfx::Size& viewport_size_for_tile_priority,
      const gfx::Transform& transform_for_tile_priority,
      bool offscreen_pre_raster,
      CopyOutputRequestQueue copy_requests);
  ~ChildFrame();

  // Helper to move frame from |frame_future| to |frame|.
  void WaitOnFutureIfNeeded();

  // The frame is either in |frame_future| or |frame|. It's illegal if both
  // are non-null.
  scoped_refptr<content::SynchronousCompositor::FrameFuture> frame_future;
  uint32_t layer_tree_frame_sink_id = 0u;
  std::unique_ptr<viz::CompositorFrame> frame;
  // The id of the compositor this |frame| comes from.
  const viz::FrameSinkId frame_sink_id;
  const gfx::Size viewport_size_for_tile_priority;
  const gfx::Transform transform_for_tile_priority;
  const bool offscreen_pre_raster;
  CopyOutputRequestQueue copy_requests;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChildFrame);
};

using ChildFrameQueue = base::circular_deque<std::unique_ptr<ChildFrame>>;

}  // namespace bison

#endif  // BISON_CORE_BROWSER_GFX_CHILD_FRAME_H_