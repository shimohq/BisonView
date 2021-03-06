// create by jiang947

#ifndef BISON_COMMON_BISON_CONTENT_CLIENT_H_
#define BISON_COMMON_BISON_CONTENT_CLIENT_H_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "content/public/common/content_client.h"

namespace gpu {
struct GPUInfo;
}

namespace bison {

class BvContentClient : public content::ContentClient {
 public:
  // ContentClient implementation.
  void AddAdditionalSchemes(Schemes* schemes) override;
  base::string16 GetLocalizedString(int message_id) override;
  base::StringPiece GetDataResource(int resource_id,
                                    ui::ScaleFactor scale_factor) override;
  base::RefCountedMemory* GetDataResourceBytes(int resource_id) override;
  void SetGpuInfo(const gpu::GPUInfo& gpu_info) override;
  bool UsingSynchronousCompositing() override;
  media::MediaDrmBridgeClient* GetMediaDrmBridgeClient() override;
  void ExposeInterfacesToBrowser(
      scoped_refptr<base::SequencedTaskRunner> io_task_runner,
      mojo::BinderMap* binders) override;
};

}  // namespace bison

#endif  // BISON_COMMON_BISON_CONTENT_CLIENT_H_
