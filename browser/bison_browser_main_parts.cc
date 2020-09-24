// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison_browser_main_parts.h"

// #include "content/shell/android/shell_descriptors.h"
#include "bison_browser_context.h"
#include "bison_contents.h"
#include "bison_devtools_manager_delegate.h"
// #include "content/shell/common/shell_switches.h"

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop_current.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "cc/base/switches.h"
#include "components/crash/content/browser/child_exit_observer_android.h"
#include "components/crash/content/browser/child_process_crash_observer_android.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"
#include "content/public/common/url_constants.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "net/android/network_change_notifier_factory_android.h"
#include "net/base/filename_util.h"
#include "net/base/net_module.h"
#include "net/base/network_change_notifier.h"
#include "net/grit/net_resources.h"
#include "services/service_manager/embedder/result_codes.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

#if defined(USE_X11)
#include "ui/base/x/x11_util.h"  // nogncheck
#endif
#if defined(USE_AURA) && defined(USE_X11)
#include "ui/events/devices/x11/touch_factory_x11.h"  // nogncheck
#endif
#if !defined(OS_CHROMEOS) && defined(USE_AURA) && defined(OS_LINUX)
#include "ui/base/ime/init/input_method_initializer.h"
#endif
#if defined(OS_CHROMEOS)
#include "chromeos/dbus/dbus_thread_manager.h"
#include "device/bluetooth/dbus/bluez_dbus_manager.h"
#elif defined(OS_LINUX)
#include "device/bluetooth/dbus/dbus_bluez_manager_wrapper_linux.h"
#endif

namespace bison {

namespace {

// GURL GetStartupURL() {
//   base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
//   if (command_line->HasSwitch(switches::kBrowserTest))
//     return GURL();
//   const base::CommandLine::StringVector& args = command_line->GetArgs();

// #if defined(OS_ANDROID)
//   // Delay renderer creation on Android until surface is ready.
//   return GURL();
// #endif

//   if (args.empty())
//     return GURL("https://www.google.com/");

//   GURL url(args[0]);
//   if (url.is_valid() && url.has_scheme())
//     return url;

//   return net::FilePathToFileURL(
//       base::MakeAbsoluteFilePath(base::FilePath(args[0])));
// }

scoped_refptr<base::RefCountedMemory> PlatformResourceProvider(int key) {
  if (key == IDR_DIR_HEADER_HTML) {
    return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
        IDR_DIR_HEADER_HTML);
  }
  return nullptr;
}

}  // namespace

BisonBrowserMainParts::BisonBrowserMainParts(
    const MainFunctionParams& parameters)
    : parameters_(parameters){
  VLOG(0) << "on new BisonBrowserMainParts ";
}

BisonBrowserMainParts::~BisonBrowserMainParts() {}

void BisonBrowserMainParts::PreMainMessageLoopStart() {
  content::RenderFrameHost::AllowInjectingJavaScript();
}

void BisonBrowserMainParts::PostMainMessageLoopStart() {}

int BisonBrowserMainParts::PreEarlyInitialization() {
  net::NetworkChangeNotifier::SetFactory(
      new net::NetworkChangeNotifierFactoryAndroid());

  return service_manager::RESULT_CODE_NORMAL_EXIT;
}

void BisonBrowserMainParts::InitializeBrowserContexts() {
  set_browser_context(new BisonBrowserContext());
}



int BisonBrowserMainParts::PreCreateThreads() {
#if defined(OS_ANDROID)
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  crash_reporter::ChildExitObserver::Create();
  if (command_line->HasSwitch(switches::kEnableCrashReporter)) {
    crash_reporter::ChildExitObserver::GetInstance()->RegisterClient(
        std::make_unique<crash_reporter::ChildProcessCrashObserver>());
  }
#endif
  return 0;
}

void BisonBrowserMainParts::PreMainMessageLoopRun() {
  InitializeBrowserContexts();

  net::NetModule::SetResourceProvider(PlatformResourceProvider);
  BisonDevToolsManagerDelegate::StartHttpHandler(browser_context_.get());

  if (parameters_.ui_task) {
    parameters_.ui_task->Run();
    delete parameters_.ui_task;
  }
}

bool BisonBrowserMainParts::MainMessageLoopRun(int* result_code) {
  return true;
}

void BisonBrowserMainParts::PostMainMessageLoopRun() {
  BisonDevToolsManagerDelegate::StopHttpHandler();
  browser_context_.reset();
}

void BisonBrowserMainParts::PreDefaultMainMessageLoopRun(
    base::OnceClosure quit_closure) {
  BisonContents::SetMainMessageLoopQuitClosure(std::move(quit_closure));
}

void BisonBrowserMainParts::PostDestroyThreads() {
#if defined(OS_CHROMEOS)
  device::BluetoothAdapterFactory::Shutdown();
  bluez::BluezDBusManager::Shutdown();
  chromeos::DBusThreadManager::Shutdown();
#elif defined(OS_LINUX)
  device::BluetoothAdapterFactory::Shutdown();
  bluez::DBusBluezManagerWrapperLinux::Shutdown();
#endif
}

}  // namespace bison
