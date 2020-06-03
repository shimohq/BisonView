// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/core/lib/bison_main_delegate.h"

#include <memory>

#include "base/android/apk_assets.h"
#include "base/android/build_info.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/cpu.h"
#include "base/i18n/icu_util.h"
#include "base/i18n/rtl.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/posix/global_descriptors.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "bison/core/browser/bison_content_browser_client.h"
#include "bison/core/browser/bison_media_url_interceptor.h"
#include "bison/core/browser/gfx/browser_view_renderer.h"
#include "bison/core/browser/gfx/gpu_service_web_view.h"
#include "bison/core/browser/gfx/viz_compositor_thread_runner_webview.h"
#include "bison/core/browser/scoped_add_feature_flags.h"
#include "bison/core/browser/tracing/bison_trace_event_args_whitelist.h"
#include "bison/core/common/bison_descriptors.h"
#include "bison/core/common/bison_features.h"
#include "bison/core/common/bison_paths.h"
#include "bison/core/common/bison_resource_bundle.h"
#include "bison/core/common/bison_switches.h"
#include "bison/core/common/crash_reporter/bison_crash_reporter_client.h"
#include "bison/core/common/crash_reporter/crash_keys.h"
#include "bison/core/gpu/bison_content_gpu_client.h"
#include "bison/core/renderer/bison_content_renderer_client.h"
#include "cc/base/switches.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/crash/core/common/crash_key.h"
#include "components/gwp_asan/buildflags/buildflags.h"
#include "components/safe_browsing/android/safe_browsing_api_handler_bridge.h"
#include "components/services/heap_profiling/public/cpp/profiling_client.h"
#include "components/spellcheck/spellcheck_buildflags.h"
#include "components/version_info/android/channel_getter.h"
#include "components/viz/common/features.h"
#include "content/public/browser/android/media_url_interceptor_register.h"
#include "content/public/browser/browser_main_runner.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_descriptor_keys.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "gin/public/isolate_holder.h"
#include "gin/v8_initializer.h"
#include "gpu/command_buffer/service/gpu_switches.h"
#include "gpu/config/gpu_finch_features.h"
#include "gpu/ipc/gl_in_process_context.h"
#include "media/base/media_switches.h"
#include "media/media_buildflags.h"
#include "ui/base/ui_base_paths.h"
#include "ui/base/ui_base_switches.h"
#include "ui/events/gesture_detection/gesture_configuration.h"

#if BUILDFLAG(ENABLE_SPELLCHECK)
#include "components/spellcheck/common/spellcheck_features.h"
#endif  // ENABLE_SPELLCHECK

#if BUILDFLAG(ENABLE_GWP_ASAN)
#include "components/gwp_asan/client/gwp_asan.h"  // nogncheck
#endif

namespace bison {

BisonMainDelegate::BisonMainDelegate() {}

BisonMainDelegate::~BisonMainDelegate() {}

bool BisonMainDelegate::BasicStartupComplete(int* exit_code) {
  content::SetContentClient(&content_client_);

  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();

  // WebView uses the Android system's scrollbars and overscroll glow.
  cl->AppendSwitch(switches::kDisableOverscrollEdgeEffect);

  // Pull-to-refresh should never be a default WebView action.
  cl->AppendSwitch(switches::kDisablePullToRefreshEffect);

  // Not yet supported in single-process mode.
  cl->AppendSwitch(switches::kDisableSharedWorkers);

  // File system API not supported (requires some new API; internal bug 6930981)
  cl->AppendSwitch(switches::kDisableFileSystem);

  // Web Notification API and the Push API are not supported (crbug.com/434712)
  cl->AppendSwitch(switches::kDisableNotifications);

  // Check damage in OnBeginFrame to prevent unnecessary draws.
  cl->AppendSwitch(cc::switches::kCheckDamageEarly);

  // This is needed for sharing textures across the different GL threads.
  cl->AppendSwitch(switches::kEnableThreadedTextureMailboxes);

  // WebView does not yet support screen orientation locking.
  cl->AppendSwitch(switches::kDisableScreenOrientationLock);

  // WebView does not currently support Web Speech Synthesis API,
  // but it does support Web Speech Recognition API (crbug.com/487255).
  cl->AppendSwitch(switches::kDisableSpeechSynthesisAPI);

  // WebView does not currently support the Permissions API (crbug.com/490120)
  cl->AppendSwitch(switches::kDisablePermissionsAPI);

  // WebView does not (yet) save Chromium data during shutdown, so add setting
  // for Chrome to aggressively persist DOM Storage to minimize data loss.
  // http://crbug.com/479767
  cl->AppendSwitch(switches::kEnableAggressiveDOMStorageFlushing);

  // Webview does not currently support the Presentation API, see
  // https://crbug.com/521319
  cl->AppendSwitch(switches::kDisablePresentationAPI);

  // WebView doesn't support Remote Playback API for the same reason as the
  // Presentation API, see https://crbug.com/521319.
  cl->AppendSwitch(switches::kDisableRemotePlaybackAPI);

  // WebView does not support MediaSession API since there's no UI for media
  // metadata and controls.
  cl->AppendSwitch(switches::kDisableMediaSessionAPI);

#if defined(V8_USE_EXTERNAL_STARTUP_DATA)
  if (cl->GetSwitchValueASCII(switches::kProcessType).empty()) {
    // Browser process (no type specified).

    content::RegisterMediaUrlInterceptor(new BisonMediaUrlInterceptor());
    BrowserViewRenderer::CalculateTileMemoryPolicy();
    // WebView apps can override WebView#computeScroll to achieve custom
    // scroll/fling. As a result, fling animations may not be ticked,
    // potentially
    // confusing the tap suppression controller. Simply disable it for WebView
    ui::GestureConfiguration::GetInstance()
        ->set_fling_touchscreen_tap_suppression_enabled(false);

    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8NativesDataDescriptor,
        gin::V8Initializer::GetNativesFilePath());
#if defined(USE_V8_CONTEXT_SNAPSHOT)
    gin::V8Initializer::V8SnapshotFileType file_type =
        gin::V8Initializer::V8SnapshotFileType::kWithAdditionalContext;
#else
    gin::V8Initializer::V8SnapshotFileType file_type =
        gin::V8Initializer::V8SnapshotFileType::kDefault;
#endif
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot32DataDescriptor,
        gin::V8Initializer::GetSnapshotFilePath(true, file_type));
    base::android::RegisterApkAssetWithFileDescriptorStore(
        content::kV8Snapshot64DataDescriptor,
        gin::V8Initializer::GetSnapshotFilePath(false, file_type));
  }
#endif  // V8_USE_EXTERNAL_STARTUP_DATA

  if (cl->HasSwitch(switches::kWebViewSandboxedRenderer)) {
    content::RenderProcessHost::SetMaxRendererProcessCount(1u);
    cl->AppendSwitch(switches::kInProcessGPU);
  }

  {
    ScopedAddFeatureFlags features(cl);

#if BUILDFLAG(ENABLE_SPELLCHECK)
    features.EnableIfNotSet(spellcheck::kAndroidSpellCheckerNonLowEnd);
#endif  // ENABLE_SPELLCHECK

    features.EnableIfNotSet(
        autofill::features::kAutofillSkipComparingInferredLabels);

    if (cl->HasSwitch(switches::kWebViewLogJsConsoleMessages)) {
      features.EnableIfNotSet(::features::kLogJsConsoleMessages);
    }

    features.DisableIfNotSet(::features::kWebPayments);

    // WebView does not and should not support WebAuthN.
    features.DisableIfNotSet(::features::kWebAuth);

    // WebView isn't compatible with OOP-D.
    features.DisableIfNotSet(::features::kVizDisplayCompositor);

    // WebView does not support AndroidOverlay yet for video overlays.
    features.DisableIfNotSet(media::kUseAndroidOverlay);

    // WebView doesn't support embedding CompositorFrameSinks which is needed
    // for UseSurfaceLayerForVideo feature. https://crbug.com/853832
    features.EnableIfNotSet(media::kDisableSurfaceLayerForVideo);

    // WebView does not support EME persistent license yet, because it's not
    // clear on how user can remove persistent media licenses from UI.
    features.DisableIfNotSet(media::kMediaDrmPersistentLicense);

    // WebView does not support Picture-in-Picture yet.
    features.DisableIfNotSet(media::kPictureInPictureAPI);

    features.DisableIfNotSet(
        autofill::features::kAutofillRestrictUnownedFieldsToFormlessCheckout);

    features.DisableIfNotSet(::features::kBackgroundFetch);

    features.DisableIfNotSet(::features::kAndroidSurfaceControl);

    // TODO(https://crbug.com/963653): SmsReceiver is not yet supported on
    // WebView.
    features.DisableIfNotSet(::features::kSmsReceiver);

    features.DisableIfNotSet(::features::kWebXr);

    // De-jelly is never supported on WebView.
    features.EnableIfNotSet(::features::kDisableDeJelly);
  }

  bison::RegisterPathProvider();

  safe_browsing_api_handler_.reset(
      new safe_browsing::SafeBrowsingApiHandlerBridge());
  safe_browsing::SafeBrowsingApiHandler::SetInstance(
      safe_browsing_api_handler_.get());

  // Used only if the argument filter is enabled in tracing config,
  // as is the case by default in aw_tracing_controller.cc
  base::trace_event::TraceLog::GetInstance()->SetArgumentFilterPredicate(
      base::BindRepeating(&IsTraceEventArgsWhitelisted));
  base::trace_event::TraceLog::GetInstance()->SetMetadataFilterPredicate(
      base::BindRepeating(&IsTraceMetadataWhitelisted));

  // The TLS slot used by the memlog allocator shim needs to be initialized
  // early to ensure that it gets assigned a low slot number. If it gets
  // initialized too late, the glibc TLS system will require a malloc call in
  // order to allocate storage for a higher slot number. Since malloc is hooked,
  // this causes re-entrancy into the allocator shim, while the TLS object is
  // partially-initialized, which the TLS object is supposed to protect again.
  heap_profiling::InitTLSSlot();

  return false;
}

void BisonMainDelegate::PreSandboxStartup() {
  // TODO jiang Sandbox
  VLOG(0) << "PreSandboxStartup()";
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

  VLOG(0) << "PreSandboxStartup() is_browser_process:" << is_browser_process;  
  if (!is_browser_process) {
    base::i18n::SetICUDefaultLocale(
        command_line.GetSwitchValueASCII(switches::kLang));
  }

  VLOG(0) << "PreSandboxStartup() process_type:" << process_type;  
  if (process_type == switches::kRendererProcess) {
    InitResourceBundleRendererSide();
  }

  // TODO jiang 这里先注释的 crashReporter
  // EnableCrashReporter(process_type);

  base::android::BuildInfo* android_build_info =
      base::android::BuildInfo::GetInstance();

  VLOG(0) << "android_build_info->host_package_name():" << android_build_info->host_package_name();

  
  // static ::crash_reporter::CrashKeyString<64> app_name_key(
  //     crash_keys::kAppPackageName);
  // app_name_key.Set(android_build_info->host_package_name());

  // static ::crash_reporter::CrashKeyString<64> app_version_key(
  //     crash_keys::kAppPackageVersionCode);
  // app_version_key.Set(android_build_info->host_version_code());

  // static ::crash_reporter::CrashKeyString<8> sdk_int_key(
  //     crash_keys::kAndroidSdkInt);
  // sdk_int_key.Set(base::NumberToString(android_build_info->sdk_int()));
}

int BisonMainDelegate::RunProcess(
    const std::string& process_type,
    const content::MainFunctionParams& main_function_params) {
  // Defer to the default main method outside the browser process.
  if (!process_type.empty())
    return -1;

  browser_runner_ = content::BrowserMainRunner::Create();
  int exit_code = browser_runner_->Initialize(main_function_params);
  // We do not expect Initialize() to ever fail in AndroidWebView. On success
  // it returns a negative value but we do not want to use that on Android.
  DCHECK_LT(exit_code, 0);
  // Return 0 so that we do NOT trigger the default behavior. On Android, the
  // UI message loop is managed by the Java application.
  return 0;
}

void BisonMainDelegate::ProcessExiting(const std::string& process_type) {
  // TODO(torne): Clean up resources when we handle them.

  logging::CloseLogFile();
}

bool BisonMainDelegate::ShouldCreateFeatureList() {
  // TODO(https://crbug.com/887468): Move the creation of FeatureList from
  // BisonBrowserMainParts::PreCreateThreads() to
  // BisonMainDelegate::PostEarlyInitialization().
  return false;
}

// This function is called only on the browser process.
void BisonMainDelegate::PostEarlyInitialization(bool is_running_tests) {
  VLOG(0) << "PostEarlyInitialization";
  InitIcuAndResourceBundleBrowserSide();
  aw_feature_list_creator_->CreateFeatureListAndFieldTrials();
  PostFieldTrialInitialization();
}

void BisonMainDelegate::PostFieldTrialInitialization() {
  VLOG(0) << "PostFieldTrialInitialization";
  version_info::Channel channel = version_info::android::GetChannel();
  bool is_canary_dev = (channel == version_info::Channel::CANARY ||
                        channel == version_info::Channel::DEV);
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line.GetSwitchValueASCII(switches::kProcessType);
  bool is_browser_process = process_type.empty();

  ALLOW_UNUSED_LOCAL(is_canary_dev);
  ALLOW_UNUSED_LOCAL(is_browser_process);

#if BUILDFLAG(ENABLE_GWP_ASAN_MALLOC)
  gwp_asan::EnableForMalloc(is_canary_dev || is_browser_process,
                            process_type.c_str());
#endif

#if BUILDFLAG(ENABLE_GWP_ASAN_PARTITIONALLOC)
  gwp_asan::EnableForPartitionAlloc(is_canary_dev, process_type.c_str());
#endif
}

content::ContentBrowserClient* BisonMainDelegate::CreateContentBrowserClient() {
  DCHECK(!aw_feature_list_creator_);
  aw_feature_list_creator_ = std::make_unique<BisonFeatureListCreator>();
  content_browser_client_.reset(
      new BisonContentBrowserClient(aw_feature_list_creator_.get()));
  return content_browser_client_.get();
}

namespace {
gpu::SyncPointManager* GetSyncPointManager() {
  DCHECK(GpuServiceWebView::GetInstance());
  return GpuServiceWebView::GetInstance()->sync_point_manager();
}

gpu::SharedImageManager* GetSharedImageManager() {
  DCHECK(GpuServiceWebView::GetInstance());
  const bool enable_shared_image =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kWebViewEnableSharedImage);
  return enable_shared_image
             ? GpuServiceWebView::GetInstance()->shared_image_manager()
             : nullptr;
}

viz::VizCompositorThreadRunner* GetVizCompositorThreadRunner() {
  return base::FeatureList::IsEnabled(features::kVizForWebView)
             ? VizCompositorThreadRunnerWebView::GetInstance()
             : nullptr;
}

}  // namespace

content::ContentGpuClient* BisonMainDelegate::CreateContentGpuClient() {
  content_gpu_client_ = std::make_unique<BisonContentGpuClient>(
      base::BindRepeating(&GetSyncPointManager),
      base::BindRepeating(&GetSharedImageManager),
      base::BindRepeating(&GetVizCompositorThreadRunner));
  return content_gpu_client_.get();
}

content::ContentRendererClient*
BisonMainDelegate::CreateContentRendererClient() {
  content_renderer_client_.reset(new BisonContentRendererClient());
  return content_renderer_client_.get();
}

}  // namespace bison
