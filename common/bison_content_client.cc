#include "bison_content_client.h"

#include "bison/common/url_constants.h"
#include "bison/grit/bison_resources.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/no_destructor.h"
#include "components/services/heap_profiling/public/cpp/profiling_client.h"
#include "content/public/common/content_switches.h"
#include "gpu/config/gpu_info.h"
#include "gpu/config/gpu_util.h"
#include "ipc/ipc_message.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace bison {

void BisonContentClient::AddAdditionalSchemes(Schemes* schemes) {
  schemes->local_schemes.push_back(url::kContentScheme);
  schemes->secure_schemes.push_back(bison::kAndroidWebViewVideoPosterScheme);
  schemes->allow_non_standard_schemes_in_origins = true;
}

base::string16 BisonContentClient::GetLocalizedString(int message_id) {
  return l10n_util::GetStringUTF16(message_id);
}

base::StringPiece BisonContentClient::GetDataResource(
    int resource_id,
    ui::ScaleFactor scale_factor) {
  return ui::ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

base::RefCountedMemory* BisonContentClient::GetDataResourceBytes(
    int resource_id) {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
      resource_id);
}

void BisonContentClient::SetGpuInfo(const gpu::GPUInfo& gpu_info) {
  gpu::SetKeysForCrashLogging(gpu_info);
}

bool BisonContentClient::UsingSynchronousCompositing() {
  // 返回 true 会渲染不出来。。
  return false;
  // return true;
}



// gfx::Image& BisonContentClient::GetNativeImageNamed(int resource_id) {
//   return ui::ResourceBundle::GetSharedInstance().GetNativeImageNamed(
//       resource_id);
// }

// blink::OriginTrialPolicy* BisonContentClient::GetOriginTrialPolicy() {
//   return &origin_trial_policy_;
// }

}  // namespace bison
