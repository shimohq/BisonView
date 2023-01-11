#include "bison/common/crash_reporter/bv_crash_reporter_client.h"

#include <stdint.h>

#include "bison/common/bv_descriptors.h"
#include "bison/common/bv_paths.h"
#include "bison/common/bv_switches.h"
#include "bison/common/crash_reporter/crash_keys.h"
#include "bison/bison_jni_headers/BvCrashReporterClient_jni.h"

#include "base/android/build_info.h"
#include "base/android/java_exception_reporter.h"
#include "base/android/jni_android.h"
#include "base/base_paths_android.h"
#include "base/base_switches.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/scoped_native_library.h"
#include "build/build_config.h"
#include "components/crash/core/app/crash_reporter_client.h"
#include "components/crash/core/app/crashpad.h"
#include "components/version_info/android/channel_getter.h"
#include "components/version_info/version_info.h"
#include "components/version_info/version_info_values.h"

using base::android::AttachCurrentThread;

namespace bison {

namespace {

class BvCrashReporterClient : public crash_reporter::CrashReporterClient {
 public:
  BvCrashReporterClient() = default;

  BvCrashReporterClient(const BvCrashReporterClient&) = delete;
  BvCrashReporterClient& operator=(const BvCrashReporterClient&) = delete;

  // crash_reporter::CrashReporterClient implementation.
  bool IsRunningUnattended() override { return false; }
  bool GetCollectStatsConsent() override {
    return false;
  }

  void GetProductNameAndVersion(std::string* product_name,
                                std::string* version,
                                std::string* channel) override {
    *product_name = "BisonView";
    *version = PRODUCT_VERSION;
    *channel =
        version_info::GetChannelString(version_info::android::GetChannel());
  }

  bool GetCrashDumpLocation(base::FilePath* crash_dir) override {
    return base::PathService::Get(bison::DIR_CRASH_DUMPS, crash_dir);
  }

  void GetSanitizationInformation(const char* const** crash_key_allowlist,
                                  void** target_module,
                                  bool* sanitize_stacks) override {
    *crash_key_allowlist = crash_keys::kWebViewCrashKeyAllowList;
#if defined(COMPONENT_BUILD)
    *target_module = nullptr;
#else
    *target_module = reinterpret_cast<void*>(&EnableCrashReporter);
#endif
    *sanitize_stacks = true;
  }

  unsigned int GetCrashDumpPercentage() override { return 100; }

  bool GetBrowserProcessType(std::string* ptype) override {
    if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kWebViewSandboxedRenderer)) {
      // In single process mode the renderer and browser are in the same
      // process. The process type is "webview" to distinguish this case,
      // and for backwards compatibility.
      *ptype = "webview";
    } else {
      // Otherwise, in multi process mode, "browser" suffices.
      *ptype = "browser";
    }
    return true;
  }

  bool ShouldWriteMinidumpToLog() override { return true; }

  bool JavaExceptionFilter(
      const base::android::JavaRef<jthrowable>& java_exception) {
    return Java_BvCrashReporterClient_stackTraceContainsWebViewCode(
        AttachCurrentThread(), java_exception);
  }

  static BvCrashReporterClient* Get() {
    static base::NoDestructor<BvCrashReporterClient> crash_reporter_client;
    return crash_reporter_client.get();
  }
};

#if defined(ARCH_CPU_X86_FAMILY)
bool SafeToUseSignalHandler() {
  // N+ shared library namespacing means that we are unable to dlopen
  // libnativebridge (because it isn't in the NDK). However we know
  // that, were we able to, the tests below would pass, so just return
  // true here.
  if (base::android::BuildInfo::GetInstance()->sdk_int() >=
      base::android::SDK_VERSION_NOUGAT) {
    return true;
  }
  // On X86/64 there are binary translators that handle SIGSEGV in userspace and
  // may get chained after our handler - see http://crbug.com/477444
  // We attempt to detect this to work out when it's safe to install breakpad.
  // If anything doesn't seem right we assume it's not safe.

  // type and mangled name of android::NativeBridgeInitialized
  typedef bool (*InitializedFunc)();
  const char kInitializedSymbol[] = "_ZN7android23NativeBridgeInitializedEv";
  // type and mangled name of android::NativeBridgeGetVersion
  typedef uint32_t (*VersionFunc)();
  const char kVersionSymbol[] = "_ZN7android22NativeBridgeGetVersionEv";

  base::ScopedNativeLibrary lib_native_bridge(
      base::FilePath("libnativebridge.so"));
  if (!lib_native_bridge.is_valid()) {
    DLOG(WARNING) << "Couldn't load libnativebridge";
    return false;
  }

  InitializedFunc NativeBridgeInitialized = reinterpret_cast<InitializedFunc>(
      lib_native_bridge.GetFunctionPointer(kInitializedSymbol));
  if (NativeBridgeInitialized == nullptr) {
    DLOG(WARNING) << "Couldn't tell if native bridge initialized";
    return false;
  }
  if (!NativeBridgeInitialized()) {
    // Native process, safe to use breakpad.
    return true;
  }

  VersionFunc NativeBridgeGetVersion = reinterpret_cast<VersionFunc>(
      lib_native_bridge.GetFunctionPointer(kVersionSymbol));
  if (NativeBridgeGetVersion == nullptr) {
    DLOG(WARNING) << "Couldn't get native bridge version";
    return false;
  }
  uint32_t version = NativeBridgeGetVersion();
  if (version >= 2) {
    // Native bridge at least version 2, safe to use breakpad.
    return true;
  } else {
    DLOG(WARNING) << "Native bridge ver=" << version << "; too low";
    return false;
  }
}
#endif

bool g_enabled;

}  // namespace

void EnableCrashReporter(const std::string& process_type) {
  if (g_enabled) {
    NOTREACHED() << "EnableCrashReporter called more than once";
    return;
  }

#if defined(ARCH_CPU_X86_FAMILY)
  if (!SafeToUseSignalHandler()) {
    LOG(WARNING) << "Can't use breakpad to handle WebView crashes";
    return;
  }
#endif

  BvCrashReporterClient* client = BvCrashReporterClient::Get();
  crash_reporter::SetCrashReporterClient(client);
  crash_reporter::InitializeCrashpad(process_type.empty(), process_type);
  if (process_type.empty()) {
    base::android::InitJavaExceptionReporter();
  } else {
    base::android::InitJavaExceptionReporterForChildProcess();
  }
  base::android::SetJavaExceptionFilter(base::BindRepeating(
      &BvCrashReporterClient::JavaExceptionFilter, base::Unretained(client)));
  g_enabled = true;
}

bool CrashReporterEnabled() {
  return g_enabled;
}

}  // namespace bison
