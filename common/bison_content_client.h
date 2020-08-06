// create by jiang947

#ifndef BISON_COMMON_BISON_CONTENT_CLIENT_H_
#define BISON_COMMON_BISON_CONTENT_CLIENT_H_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "content/public/common/content_client.h"

namespace bison {

class BisonContentClient : public content::ContentClient {
 public:
  BisonContentClient();
  ~BisonContentClient() override;

  base::string16 GetLocalizedString(int message_id) override;
  base::StringPiece GetDataResource(int resource_id,
                                    ui::ScaleFactor scale_factor) override;
  base::RefCountedMemory* GetDataResourceBytes(int resource_id) override;
  gfx::Image& GetNativeImageNamed(int resource_id) override;
  base::DictionaryValue GetNetLogConstants() override;
  // blink::OriginTrialPolicy* GetOriginTrialPolicy() override;

  //  private:
  //   ShellOriginTrialPolicy origin_trial_policy_;
};

}  // namespace bison

#endif  // BISON_COMMON_BISON_CONTENT_CLIENT_H_
