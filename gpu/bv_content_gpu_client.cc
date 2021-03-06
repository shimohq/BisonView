#include "bison/gpu/bv_content_gpu_client.h"

namespace bison {

BvContentGpuClient::BvContentGpuClient(
    const GetSyncPointManagerCallback &sync_point_manager_callback,
    const GetSharedImageManagerCallback &shared_image_manager_callback)
    : sync_point_manager_callback_(sync_point_manager_callback),
      shared_image_manager_callback_(shared_image_manager_callback) {}

BvContentGpuClient::BvContentGpuClient() = default;

BvContentGpuClient::~BvContentGpuClient() = default;

gpu::SyncPointManager *BvContentGpuClient::GetSyncPointManager() {
  if (!sync_point_manager_callback_.is_null())
    return sync_point_manager_callback_.Run();
  return nullptr;
}

gpu::SharedImageManager *BvContentGpuClient::GetSharedImageManager() {
  if (!shared_image_manager_callback_.is_null())
    return shared_image_manager_callback_.Run();
  return nullptr;
}

} // namespace bison
