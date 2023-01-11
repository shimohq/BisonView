#include "bv_content_client.h"

#include "bison/common/bv_features.h"
#include "bison/common/bv_media_drm_bridge_client.h"
#include "bison/common/crash_reporter/crash_keys.h"
#include "bison/common/url_constants.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/debug/crash_logging.h"
#include "base/no_destructor.h"
#include "components/embedder_support/origin_trials/origin_trial_policy_impl.h"
#include "components/services/heap_profiling/public/cpp/profiling_client.h"
#include "components/version_info/version_info.h"
#include "content/public/common/content_switches.h"
#include "gpu/config/gpu_info.h"
#include "gpu/config/gpu_util.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace bison {

BvContentClient::BvContentClient() = default;
BvContentClient::~BvContentClient() = default;

void BvContentClient::AddAdditionalSchemes(Schemes* schemes) {
  schemes->local_schemes.push_back(url::kContentScheme);
  schemes->secure_schemes.push_back(bison::kAndroidWebViewVideoPosterScheme);
  schemes->csp_bypassing_schemes.push_back(
      bison::kAndroidWebViewVideoPosterScheme);
  schemes->allow_non_standard_schemes_in_origins = true;
}

std::u16string BvContentClient::GetLocalizedString(int message_id) {
  return l10n_util::GetStringUTF16(message_id);
}

base::StringPiece BvContentClient::GetDataResource(
    int resource_id,
    ui::ResourceScaleFactor scale_factor) {
  // TODO(boliu): Used only by WebKit, so only bundle those resources for
  // Android WebView.
  return ui::ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

base::RefCountedMemory* BvContentClient::GetDataResourceBytes(int resource_id) {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
      resource_id);
}

std::string BvContentClient::GetDataResourceString(int resource_id) {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
      resource_id);
}

void BvContentClient::SetGpuInfo(const gpu::GPUInfo& gpu_info) {
  gpu::SetKeysForCrashLogging(gpu_info);
}

// 与Android WebView 不同
bool BvContentClient::UsingSynchronousCompositing() {
  return false;
}

media::MediaDrmBridgeClient* BvContentClient::GetMediaDrmBridgeClient() {
  return new BvMediaDrmBridgeClient();
}

void BvContentClient::ExposeInterfacesToBrowser(
    scoped_refptr<base::SequencedTaskRunner> io_task_runner,
    mojo::BinderMap* binders) {
  // This creates a process-wide heap_profiling::ProfilingClient that listens
  // for requests from the HeapProfilingService to start profiling the current
  // process.
  binders->Add(
      base::BindRepeating(
          [](mojo::PendingReceiver<heap_profiling::mojom::ProfilingClient>
                 receiver) {
            static base::NoDestructor<heap_profiling::ProfilingClient>
                profiling_client;
            profiling_client->BindToInterface(std::move(receiver));
          }),
      io_task_runner);
}

blink::OriginTrialPolicy* BvContentClient::GetOriginTrialPolicy() {
  if (!base::FeatureList::IsEnabled(features::kWebViewOriginTrials)) {
    return nullptr;
  }

  // Prevent initialization race (see crbug.com/721144). There may be a
  // race when the policy is needed for worker startup (which happens on a
  // separate worker thread).
  base::AutoLock auto_lock(origin_trial_policy_lock_);
  if (!origin_trial_policy_)
    origin_trial_policy_ =
        std::make_unique<embedder_support::OriginTrialPolicyImpl>();
  return origin_trial_policy_.get();
}

}  // namespace bison
