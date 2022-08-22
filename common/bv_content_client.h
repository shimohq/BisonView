// create by jiang947

#ifndef BISON_COMMON_BISON_CONTENT_CLIENT_H_
#define BISON_COMMON_BISON_CONTENT_CLIENT_H_

#include "base/synchronization/lock.h"
#include "content/public/common/content_client.h"

#include "base/compiler_specific.h"

namespace embedder_support {
class OriginTrialPolicyImpl;
}

namespace gpu {
struct GPUInfo;
}

namespace bison {

class BvContentClient : public content::ContentClient {
 public:
  BvContentClient();
  ~BvContentClient() override;
  // ContentClient implementation.
  void AddAdditionalSchemes(Schemes* schemes) override;
  std::u16string GetLocalizedString(int message_id) override;
  base::StringPiece GetDataResource(
      int resource_id,
      ui::ResourceScaleFactor scale_factor) override;
  base::RefCountedMemory* GetDataResourceBytes(int resource_id) override;
  std::string GetDataResourceString(int resource_id) override;
  void SetGpuInfo(const gpu::GPUInfo& gpu_info) override;
  bool UsingSynchronousCompositing() override;
  media::MediaDrmBridgeClient* GetMediaDrmBridgeClient() override;
  void ExposeInterfacesToBrowser(
      scoped_refptr<base::SequencedTaskRunner> io_task_runner,
      mojo::BinderMap* binders) override;
  blink::OriginTrialPolicy* GetOriginTrialPolicy() override;

 private:
  // Used to lock when |origin_trial_policy_| is initialized.
  base::Lock origin_trial_policy_lock_;
  std::unique_ptr<embedder_support::OriginTrialPolicyImpl> origin_trial_policy_;
};

}  // namespace bison

#endif  // BISON_COMMON_BISON_CONTENT_CLIENT_H_
