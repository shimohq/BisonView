// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/browser/bison_browser_main_parts.h"

#include <memory>
#include <set>
#include <string>
#include <utility>

#include "bison/android/browser/bison_browser_context.h"
#include "bison/android/browser/bison_browser_terminator.h"
#include "bison/android/browser/bison_content_browser_client.h"
#include "bison/android/browser/bison_web_ui_controller_factory.h"
#include "bison/android/browser/metrics/bison_metrics_service_client.h"
#include "bison/android/browser/metrics/memory_metrics_logger.h"
#include "bison/android/browser/network_service/bison_network_change_notifier_factory.h"
#include "bison/android/common/bison_descriptors.h"
#include "bison/android/common/bison_paths.h"
#include "bison/android/common/bison_resource.h"
#include "bison/android/common/bison_switches.h"
#include "bison/android/common/crash_reporter/bison_crash_reporter_client.h"
#include "base/android/apk_assets.h"
#include "base/android/build_info.h"
#include "base/android/memory_pressure_listener_android.h"
#include "base/base_paths_android.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/i18n/rtl.h"
#include "base/message_loop/message_loop_current.h"
#include "base/message_loop/message_pump_type.h"
#include "base/path_service.h"
#include "components/crash/content/browser/child_exit_observer_android.h"
#include "components/heap_profiling/supervisor.h"
#include "components/services/heap_profiling/public/cpp/settings.h"
#include "components/user_prefs/user_prefs.h"
#include "components/variations/variations_crash_keys.h"
#include "content/public/browser/android/synchronous_compositor.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/result_codes.h"
#include "content/public/common/service_names.mojom.h"
#include "net/android/network_change_notifier_factory_android.h"
#include "net/base/network_change_notifier.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/layout.h"
#include "ui/gl/gl_surface.h"

namespace bison {

BisonBrowserMainParts::BisonBrowserMainParts(BisonContentBrowserClient* browser_client)
    : browser_client_(browser_client) {
}

BisonBrowserMainParts::~BisonBrowserMainParts() {
}

int BisonBrowserMainParts::PreEarlyInitialization() {
  // Network change notifier factory must be singleton, only set factory
  // instance while it is not been created.
  // In most cases, this check is not necessary because SetFactory should be
  // called only once, but both webview and native cronet calls this function,
  // in case of building both webview and cronet to one app, it is required to
  // avoid crashing the app.
  if (!net::NetworkChangeNotifier::GetFactory()) {
    net::NetworkChangeNotifier::SetFactory(
        new BisonNetworkChangeNotifierFactory());
  }

  // Creates a SingleThreadTaskExecutor for Android WebView if doesn't exist.
  DCHECK(!main_task_executor_.get());
  if (!base::MessageLoopCurrent::IsSet()) {
    main_task_executor_ = std::make_unique<base::SingleThreadTaskExecutor>(
        base::MessagePumpType::UI);
  }

  browser_process_ = std::make_unique<BisonBrowserProcess>(
      browser_client_->aw_feature_list_creator());
  return service_manager::RESULT_CODE_NORMAL_EXIT;
}

int BisonBrowserMainParts::PreCreateThreads() {
  base::android::MemoryPressureListenerAndroid::Initialize(
      base::android::AttachCurrentThread());
  ::crash_reporter::ChildExitObserver::Create();

  // We need to create the safe browsing specific directory even if the
  // BisonSafeBrowsingConfigHelper::GetSafeBrowsingEnabled() is false
  // initially, because safe browsing can be enabled later at runtime
  // on a per-webview basis.
  base::FilePath safe_browsing_dir;
  if (base::PathService::Get(bison::DIR_SAFE_BROWSING,
                             &safe_browsing_dir)) {
    if (!base::PathExists(safe_browsing_dir))
      base::CreateDirectory(safe_browsing_dir);
  }

  base::FilePath crash_dir;
  if (base::PathService::Get(bison::DIR_CRASH_DUMPS, &crash_dir)) {
    if (!base::PathExists(crash_dir)) {
      base::CreateDirectory(crash_dir);
    }
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kWebViewSandboxedRenderer)) {
    // Create the renderers crash manager on the UI thread.
    ::crash_reporter::ChildExitObserver::GetInstance()->RegisterClient(
        std::make_unique<BisonBrowserTerminator>());
  }

  variations::InitCrashKeys();

  return service_manager::RESULT_CODE_NORMAL_EXIT;
}

void BisonBrowserMainParts::PreMainMessageLoopRun() {
  BisonBrowserProcess::GetInstance()->PreMainMessageLoopRun();
  browser_client_->InitBrowserContext();
  content::WebUIControllerFactory::RegisterFactory(
      BisonWebUIControllerFactory::GetInstance());
  content::RenderFrameHost::AllowInjectingJavaScript();
  metrics_logger_ = std::make_unique<MemoryMetricsLogger>();
}

bool BisonBrowserMainParts::MainMessageLoopRun(int* result_code) {
  // Android WebView does not use default MessageLoop. It has its own
  // Android specific MessageLoop.
  return true;
}

void BisonBrowserMainParts::PostCreateThreads() {
  heap_profiling::Mode mode = heap_profiling::GetModeForStartup();
  if (mode != heap_profiling::Mode::kNone)
    heap_profiling::Supervisor::GetInstance()->Start(base::NullCallback());
}

}  // namespace bison
