
#include "bv_main_delegate.h"

#include <iostream>

#include "bison/browser/bv_content_browser_client.h"
#include "bison/browser/bv_media_url_interceptor.h"
#include "bison/common/bv_content_client.h"
#include "bison/common/bv_descriptors.h"
#include "bison/common/bv_paths.h"
#include "bison/common/bv_resource_bundle.h"
#include "bison/common/bv_switches.h"
#include "bison/common/crash_reporter/bv_crash_reporter_client.h"
#include "bison/common/crash_reporter/crash_keys.h"
#include "bison/gpu/bv_content_gpu_client.h"
#include "bison/renderer/bv_content_renderer_client.h"

#include "base/android/apk_assets.h"
#include "base/android/build_info.h"
#include "base/bind.h"
#include "base/check_op.h"
#include "base/command_line.h"
#include "base/cpu.h"
#include "base/i18n/icu_util.h"
#include "base/i18n/rtl.h"
#include "base/posix/global_descriptors.h"
#include "base/scoped_add_feature_flags.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "cc/base/switches.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/crash/core/common/crash_key.h"
#include "components/embedder_support/switches.h"
#include "components/gwp_asan/buildflags/buildflags.h"
#include "components/metrics/unsent_log_store_metrics.h"
#include "components/services/heap_profiling/public/cpp/profiling_client.h"
#include "components/spellcheck/spellcheck_buildflags.h"
#include "components/translate/core/common/translate_util.h"
#include "components/variations/variations_ids_provider.h"
#include "components/version_info/android/channel_getter.h"
#include "components/viz/common/features.h"
#include "content/public/app/initialize_mojo_core.h"
#include "content/public/browser/android/compositor.h"
#include "content/public/browser/android/media_url_interceptor_register.h"
#include "content/public/browser/browser_main_runner.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_descriptor_keys.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"
#include "device/base/features.h"
#include "gin/public/isolate_holder.h"
#include "gin/v8_initializer.h"
#include "gpu/command_buffer/service/gpu_switches.h"
#include "gpu/config/gpu_finch_features.h"
#include "media/base/media_switches.h"
#include "media/media_buildflags.h"
#include "net/base/features.h"
#include "services/network/public/cpp/features.h"
#include "third_party/abseil-cpp/absl/types/variant.h"
#include "third_party/blink/public/common/features.h"
#include "ui/base/ui_base_paths.h"
#include "ui/base/ui_base_switches.h"
#include "ui/events/gesture_detection/gesture_configuration.h"
#include "ui/gl/gl_switches.h"

#if BUILDFLAG(ENABLE_SPELLCHECK)
#include "components/spellcheck/common/spellcheck_features.h"
#endif  // ENABLE_SPELLCHECK

#if BUILDFLAG(ENABLE_GWP_ASAN)
#include "components/gwp_asan/client/gwp_asan.h"  // nogncheck
#endif

namespace bison {

BvMainDelegate::BvMainDelegate() = default;

BvMainDelegate::~BvMainDelegate() = default;

absl::optional<int> BvMainDelegate::BasicStartupComplete() {
  //TRACE_EVENT0("startup", "BvMainDelegate::BasicStartupComplete");
  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();

  cl->AppendSwitch(switches::kDisableOverscrollEdgeEffect);

  cl->AppendSwitch(switches::kDisablePullToRefreshEffect);

  // Not yet supported in single-process mode.
  cl->AppendSwitch(switches::kDisableSharedWorkers);

  cl->AppendSwitch(switches::kDisableFileSystem);

  // Web Notification API and the Push API are not supported (crbug.com/434712)
  cl->AppendSwitch(switches::kDisableNotifications);

  // Check damage in OnBeginFrame to prevent unnecessary draws.
  cl->AppendSwitch(cc::switches::kCheckDamageEarly);

  // This is needed for sharing textures across the different GL threads.
  cl->AppendSwitch(switches::kEnableThreadedTextureMailboxes);

  // WebView does not yet support screen orientation locking.
  cl->AppendSwitch(switches::kDisableScreenOrientationLock);
  cl->AppendSwitch(switches::kDisableSpeechSynthesisAPI);

  cl->AppendSwitch(switches::kDisablePermissionsAPI);

  cl->AppendSwitch(switches::kEnableAggressiveDOMStorageFlushing);

  cl->AppendSwitch(switches::kDisablePresentationAPI);

  cl->AppendSwitch(switches::kDisableRemotePlaybackAPI);

  cl->AppendSwitch(switches::kDisableMediaSessionAPI);

  // We have crash dumps to diagnose regressions in remote font analysis or cc
  // serialization errors but most of their utility is in identifying URLs where
  // the regression occurs. This info is not available for webview so there
  // isn't much point in having the crash dumps there.
  cl->AppendSwitch(switches::kDisableOoprDebugCrashDump);

  // Disable BackForwardCache for Android WebView as it is not supported.
  // WebView-specific code hasn't been audited and fixed to ensure compliance
  // with the changed API contracts around new navigation types and changes to
  // the document lifecycle.
  cl->AppendSwitch(switches::kDisableBackForwardCache);

  // Deemed that performance benefit is not worth the stability cost.
  // See crbug.com/1309151.
  cl->AppendSwitch(switches::kDisableGpuShaderDiskCache);

  // ref :https://bugs.chromium.org/p/chromium/issues/detail?id=1306508
  //cl->AppendSwitch(switches::kDisableRendererAccessibility);

  if (cl->GetSwitchValueASCII(switches::kProcessType).empty()) {
    // Browser process (no type specified).

    content::RegisterMediaUrlInterceptor(new BvMediaUrlInterceptor());
    // BrowserViewRenderer::CalculateTileMemoryPolicy();

    // WebView apps can override WebView#computeScroll to achieve custom
    // scroll/fling. As a result, fling animations may not be ticked,
    // potentially
    // confusing the tap suppression controller. Simply disable it for WebView
    ui::GestureConfiguration::GetInstance()
        ->set_fling_touchscreen_tap_suppression_enabled(false);

#if defined(USE_V8_CONTEXT_SNAPSHOT)
#if defined(ARCH_CPU_ARM_FAMILY)
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot32DataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/arm/v8_context_snapshot_32.bin"));
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot64DataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/arm/v8_context_snapshot_64.bin"));
#else
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot32DataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/x86/v8_context_snapshot_32.bin"));
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot64DataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/x86/v8_context_snapshot_64.bin"));
#endif  // ARCH_CPU_ARM_FAMILY
#else
#if defined(ARCH_CPU_ARM_FAMILY)
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot32DataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/arm/snapshot_blob_32.bin"));
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot64DataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/arm/snapshot_blob_64.bin"));
#else
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot32DataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/x86/snapshot_blob_32.bin"));
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot64DataDescriptor,
        base::FilePath(FILE_PATH_LITERAL("assets"))
            .AppendASCII("bison/x86/snapshot_blob_64.bin"));
#endif  // ARCH_CPU_ARM_FAMILY
#endif
  }

  if (cl->HasSwitch(switches::kWebViewSandboxedRenderer)) {
    cl->AppendSwitch(switches::kInProcessGPU);
  }

  {
    base::ScopedAddFeatureFlags features(cl);

    if (base::android::BuildInfo::GetInstance()->sdk_int() >=
        base::android::SDK_VERSION_OREO) {
      features.EnableIfNotSet(autofill::features::kAutofillExtractAllDatalists);
      features.EnableIfNotSet(
          autofill::features::kAutofillSkipComparingInferredLabels);
    }

    if (cl->HasSwitch(switches::kWebViewLogJsConsoleMessages)) {
      features.EnableIfNotSet(::features::kLogJsConsoleMessages);
    }

    if (!cl->HasSwitch(switches::kWebViewDrawFunctorUsesVulkan)) {
      // Not use ANGLE's Vulkan backend, if the draw functor is not using
      // Vulkan.
      features.DisableIfNotSet(::features::kDefaultANGLEVulkan);
    }

    if (cl->HasSwitch(switches::kWebViewMPArchFencedFrames)) {
      features.EnableIfNotSetWithParameter(blink::features::kFencedFrames,
                                           "implementation_type", "mparch");
      features.EnableIfNotSet(blink::features::kSharedStorageAPI);
      features.EnableIfNotSet(::features::kPrivacySandboxAdsAPIsOverride);
    }

    if (cl->HasSwitch(switches::kWebViewShadowDOMFencedFrames)) {
      features.EnableIfNotSetWithParameter(blink::features::kFencedFrames,
                                           "implementation_type", "shadow_dom");
      features.EnableIfNotSet(blink::features::kSharedStorageAPI);
      features.EnableIfNotSet(::features::kPrivacySandboxAdsAPIsOverride);
    }

    // WebView uses kWebViewVulkan to control vulkan. Pre-emptively disable
    // kVulkan in case it becomes enabled by default.
    features.DisableIfNotSet(::features::kVulkan);

    features.DisableIfNotSet(::features::kWebPayments);
    features.DisableIfNotSet(::features::kServiceWorkerPaymentApps);

    // WebView does not support overlay fullscreen yet for video overlays.
    features.DisableIfNotSet(media::kOverlayFullscreenVideo);

    features.DisableIfNotSet(media::kMediaDrmPersistentLicense);

    features.DisableIfNotSet(media::kPictureInPictureAPI);

    features.DisableIfNotSet(::features::kBackgroundFetch);

    // SurfaceControl is controlled by kWebViewSurfaceControl flag.
    features.DisableIfNotSet(::features::kAndroidSurfaceControl);

    // TODO(https://crbug.com/963653): WebOTP is not yet supported on
    // WebView.
    features.DisableIfNotSet(::features::kWebOTP);

    // TODO(https://crbug.com/1012899): WebXR is not yet supported on WebView.
    features.DisableIfNotSet(::features::kWebXr);

    features.DisableIfNotSet(::features::kWebXrArModule);

    features.DisableIfNotSet(device::features::kWebXrHitTest);

    // TODO(https://crbug.com/1312827): Digital Goods API is not yet supported
    // on WebView.
    features.DisableIfNotSet(::features::kDigitalGoodsApi);

    features.DisableIfNotSet(::features::kDynamicColorGamut);

    // De-jelly is never supported on WebView.
    features.EnableIfNotSet(::features::kDisableDeJelly);

    // COOP is not supported on WebView yet. See:
    // https://groups.google.com/a/chromium.org/forum/#!topic/blink-dev/XBKAGb2_7uAi.
    features.DisableIfNotSet(network::features::kCrossOriginOpenerPolicy);

    features.DisableIfNotSet(::features::kInstalledApp);
    // features.EnableIfNotSet(
    //     metrics::UnsentLogStoreMetrics::kRecordLastUnsentLogMetadataMetrics);
    features.DisableIfNotSet(::features::kPeriodicBackgroundSync);

    // TODO(crbug.com/921655): Add support for User Agent Client hints on
    // WebView.
    features.DisableIfNotSet(blink::features::kUserAgentClientHint);

    // Disable Reducing User Agent minor version on WebView.
    features.DisableIfNotSet(blink::features::kReduceUserAgentMinorVersion);

    // Disabled until viz scheduling can be improved.
    features.DisableIfNotSet(::features::kUseSurfaceLayerForVideoDefault);

    // Enabled by default for webview.
    features.EnableIfNotSet(::features::kWebViewThreadSafeMediaDefault);

    // Disable dr-dc on webview.
    features.DisableIfNotSet(::features::kEnableDrDc);

    // TODO(crbug.com/1100993): Web Bluetooth is not yet supported on WebView.
    features.DisableIfNotSet(::features::kWebBluetooth);

    // TODO(crbug.com/933055): WebUSB is not yet supported on WebView.
    features.DisableIfNotSet(::features::kWebUsb);

    // Disable TFLite based language detection on webview until webview supports
    // ML model delivery via Optimization Guide component.
    // TODO(crbug.com/1292622): Enable the feature on Webview.
    features.DisableIfNotSet(::translate::kTFLiteLanguageDetectionEnabled);

    // Disable key pinning enforcement on webview.
    features.DisableIfNotSet(net::features::kStaticKeyPinningEnforcement);

    // Have the network service in the browser process even if we have separate
    // renderer processes. See also: switches::kInProcessGPU above.
    features.EnableIfNotSet(::features::kNetworkServiceInProcess);

    // Disable Event.path on Canary and Dev to help the deprecation and removal.
    // See crbug.com/1277431 for more details.
    if (version_info::android::GetChannel() < version_info::Channel::BETA)
      features.DisableIfNotSet(blink::features::kEventPath);

    // FedCM is not yet supported on WebView.
    features.DisableIfNotSet(::features::kFedCm);
  }

  bison::RegisterPathProvider();
  content::Compositor::Initialize();
  heap_profiling::InitTLSSlot();

  return absl::nullopt;
}

void BvMainDelegate::PreSandboxStartup() {
  //TRACE_EVENT0("startup", "BvMainDelegate::PreSandboxStartup");
#if defined(ARCH_CPU_ARM_FAMILY)
  // Create an instance of the CPU class to parse /proc/cpuinfo and cache
  // cpu_brand info.
  base::CPU cpu_info;
#endif

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  std::string process_type =
      command_line.GetSwitchValueASCII(switches::kProcessType);
  const bool is_browser_process = process_type.empty();
  if (!is_browser_process) {
    base::i18n::SetICUDefaultLocale(
        command_line.GetSwitchValueASCII(switches::kLang));
  }

  if (process_type == switches::kRendererProcess) {
    InitResourceBundleRendererSide();
  }

  EnableCrashReporter(process_type);

  base::android::BuildInfo* android_build_info =
    base::android::BuildInfo::GetInstance();

  static ::crash_reporter::CrashKeyString<64> app_name_key(
      crash_keys::kAppPackageName);
  app_name_key.Set(android_build_info->host_package_name());

  static ::crash_reporter::CrashKeyString<64> app_version_key(
      crash_keys::kAppPackageVersionCode);
  app_version_key.Set(android_build_info->host_version_code());

  static ::crash_reporter::CrashKeyString<8> sdk_int_key(
      crash_keys::kAndroidSdkInt);
  sdk_int_key.Set(base::NumberToString(android_build_info->sdk_int()));
}

absl::variant<int, content::MainFunctionParams> BvMainDelegate::RunProcess(
    const std::string& process_type,
    content::MainFunctionParams main_function_params) {
  // Defer to the default main method outside the browser process.
  if (!process_type.empty())
    return std::move(main_function_params);

  browser_runner_ = content::BrowserMainRunner::Create();
  int exit_code = browser_runner_->Initialize(std::move(main_function_params));
  // We do not expect Initialize() to ever fail in AndroidWebView. On success
  // it returns a negative value but we do not want to use that on Android.
  DCHECK_LT(exit_code, 0);
  return 0;
}

void BvMainDelegate::ProcessExiting(const std::string& process_type) {
  logging::CloseLogFile();
}

bool BvMainDelegate::ShouldCreateFeatureList(InvokedIn invoked_in) {
  return absl::holds_alternative<InvokedInChildProcess>(invoked_in);
}

bool BvMainDelegate::ShouldInitializeMojo(InvokedIn invoked_in) {
  return ShouldCreateFeatureList(invoked_in);
}

variations::VariationsIdsProvider*
BvMainDelegate::CreateVariationsIdsProvider() {
  return variations::VariationsIdsProvider::Create(
      variations::VariationsIdsProvider::Mode::kDontSendSignedInVariations);
}

absl::optional<int> BvMainDelegate::PostEarlyInitialization(
    InvokedIn invoked_in) {
  const bool is_browser_process =
      absl::holds_alternative<InvokedInBrowserProcess>(invoked_in);
  if (is_browser_process) {
    InitIcuAndResourceBundleBrowserSide();
    bv_feature_list_creator_->CreateFeatureListAndFieldTrials();
    content::InitializeMojoCore();
  }

  [[maybe_unused]] const std::string process_type =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kProcessType);

#if BUILDFLAG(ENABLE_GWP_ASAN_MALLOC)
  gwp_asan::EnableForMalloc(is_browser_process, process_type.c_str());
#endif

#if BUILDFLAG(ENABLE_GWP_ASAN_PARTITIONALLOC)
  gwp_asan::EnableForPartitionAlloc(is_browser_process, process_type.c_str());
#endif
  return absl::nullopt;
}

content::ContentClient* BvMainDelegate::CreateContentClient() {
  return &content_client_;
}

ContentBrowserClient* BvMainDelegate::CreateContentBrowserClient() {
  bv_feature_list_creator_ = std::make_unique<BvFeatureListCreator>();
  browser_client_ =
      std::make_unique<BvContentBrowserClient>(bv_feature_list_creator_.get());
  return browser_client_.get();
}

ContentGpuClient* BvMainDelegate::CreateContentGpuClient() {
  gpu_client_ = std::make_unique<BvContentGpuClient>();
  return gpu_client_.get();
}

ContentRendererClient* BvMainDelegate::CreateContentRendererClient() {
  renderer_client_ = std::make_unique<BvContentRendererClient>();
  return renderer_client_.get();
}

}  // namespace bison
