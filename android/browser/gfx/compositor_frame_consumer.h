// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_BROWSER_GFX_COMPOSITOR_FRAME_CONSUMER_H_
#define BISON_ANDROID_BROWSER_GFX_COMPOSITOR_FRAME_CONSUMER_H_

#include "bison/android/browser/gfx/child_frame.h"
#include "bison/android/browser/gfx/parent_compositor_draw_constraints.h"
#include "components/viz/common/frame_timing_details_map.h"
#include "ui/gfx/geometry/vector2d.h"

namespace viz {
class FrameSinkId;
}

namespace bison {

class ChildFrame;
class CompositorFrameProducer;

class CompositorFrameConsumer {
 public:
  // A CompositorFrameConsumer may be registered with at most one
  // CompositorFrameProducer.
  // The producer is responsible for managing the relationship with its
  // consumers. The only exception to this is when a consumer is explicitly
  // destroyed, at which point it needs to inform its producer.
  // In order to register a consumer with a new producer, the current producer
  // must unregister the consumer, and call SetCompositorProducer(nullptr).
  virtual void SetCompositorFrameProducer(
      CompositorFrameProducer* compositor_frame_producer) = 0;
  virtual void SetScrollOffsetOnUI(gfx::Vector2d scroll_offset) = 0;
  // Returns uncommitted frame to be returned, if any.
  virtual std::unique_ptr<ChildFrame> SetFrameOnUI(
      std::unique_ptr<ChildFrame> frame) = 0;
  virtual void TakeParentDrawDataOnUI(
      ParentCompositorDrawConstraints* constraints,
      viz::FrameSinkId* frame_sink_id,
      viz::FrameTimingDetailsMap* timing_details,
      uint32_t* frame_token) = 0;
  virtual ChildFrameQueue PassUncommittedFrameOnUI() = 0;

 protected:
  virtual ~CompositorFrameConsumer() {}
};

}  // namespace bison

#endif  // BISON_ANDROID_BROWSER_GFX_COMPOSITOR_FRAME_CONSUMER_H_
