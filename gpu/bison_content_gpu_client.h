// create by jiang947

#ifndef BISON_GPU_BISON_CONTENT_GPU_CLIENT_H_
#define BISON_GPU_BISON_CONTENT_GPU_CLIENT_H_

#include <memory>

#include "base/macros.h"
#include "content/public/gpu/content_gpu_client.h"

using content::ContentGpuClient;

namespace bison {

class BisonContentGpuClient : public ContentGpuClient {
 public:
  BisonContentGpuClient();
  ~BisonContentGpuClient() override;

  DISALLOW_COPY_AND_ASSIGN(BisonContentGpuClient);
};

}  // namespace bison

#endif  // BISON_GPU_BISON_CONTENT_GPU_CLIENT_H_
