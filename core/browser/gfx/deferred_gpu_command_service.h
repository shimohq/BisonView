// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_BROWSER_GFX_DEFERRED_GPU_COMMAND_SERVICE_H_
#define BISON_CORE_BROWSER_GFX_DEFERRED_GPU_COMMAND_SERVICE_H_

#include "base/macros.h"
#include "gpu/ipc/command_buffer_task_executor.h"

namespace bison {

class GpuServiceWebView;
class TaskQueueWebView;

// Implementation for gpu service objects accessor for command buffer WebView.
class DeferredGpuCommandService : public gpu::CommandBufferTaskExecutor {
 public:
  static DeferredGpuCommandService* GetInstance();

  // gpu::CommandBufferTaskExecutor implementation.
  bool ForceVirtualizedGLContexts() const override;
  bool ShouldCreateMemoryTracker() const override;
  std::unique_ptr<gpu::SingleTaskSequence> CreateSequence() override;
  void ScheduleOutOfOrderTask(base::OnceClosure task) override;
  void ScheduleDelayedWork(base::OnceClosure task) override;
  void PostNonNestableToClient(base::OnceClosure callback) override;

 protected:
  ~DeferredGpuCommandService() override;

 private:
  friend class TaskForwardingSequence;

  DeferredGpuCommandService(TaskQueueWebView* task_queue,
                            GpuServiceWebView* gpu_service);

  static DeferredGpuCommandService* CreateDeferredGpuCommandService();

  TaskQueueWebView* task_queue_;
  GpuServiceWebView* gpu_service_;

  DISALLOW_COPY_AND_ASSIGN(DeferredGpuCommandService);
};

}  // namespace bison

#endif  // BISON_CORE_BROWSER_GFX_DEFERRED_GPU_COMMAND_SERVICE_H_
