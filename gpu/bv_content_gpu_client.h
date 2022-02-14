// create by jiang947

#ifndef BISON_GPU_BISON_CONTENT_GPU_CLIENT_H_
#define BISON_GPU_BISON_CONTENT_GPU_CLIENT_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "content/public/gpu/content_gpu_client.h"

using content::ContentGpuClient;

namespace bison {

class BvContentGpuClient : public ContentGpuClient {
  using GetSyncPointManagerCallback = base::RepeatingCallback<gpu::SyncPointManager*()>;
  using GetSharedImageManagerCallback = base::RepeatingCallback<gpu::SharedImageManager*()>;

 public:
  BvContentGpuClient(const GetSyncPointManagerCallback& sync_point_manager_callback,
                        const GetSharedImageManagerCallback& shared_image_manager_callback);
  BvContentGpuClient();
  ~BvContentGpuClient() override;

  // content::ContentGpuClient implementation.
  gpu::SyncPointManager* GetSyncPointManager() override;
  gpu::SharedImageManager* GetSharedImageManager() override;

private:
  GetSyncPointManagerCallback sync_point_manager_callback_;
  GetSharedImageManagerCallback shared_image_manager_callback_;
  // DISALLOW_COPY_AND_ASSIGN(BvContentGpuClient);
};

}  // namespace bison

#endif  // BISON_GPU_BISON_CONTENT_GPU_CLIENT_H_
