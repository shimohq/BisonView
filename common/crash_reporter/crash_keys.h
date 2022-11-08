
#ifndef BISON_COMMON_CRASH_REPORTER_CRASH_KEYS_H_
#define BISON_COMMON_CRASH_REPORTER_CRASH_KEYS_H_

namespace bison {
namespace crash_keys {

// Registers all of the potential crash keys that can be sent to the crash
// reporting server. Returns the size of the union of all keys.
void InitCrashKeysForWebViewTesting();

extern const char* const kWebViewCrashKeyAllowList[];

// Crash Key Name Constants ////////////////////////////////////////////////////

// Application information.
extern const char kAppPackageName[];
extern const char kAppPackageVersionCode[];
extern const char kAppProcessName[];

extern const char kAndroidSdkInt[];

extern const char kSupportLibraryWebkitVersion[];

// Indicates whether weblayer and webview are running in the same process.
// When this is true, crashes may be reported to both WebLayer and WebView,
// regardless of whetere the crash happened.
extern const char kWeblayerWebViewCompatMode[];

}  // namespace crash_keys
}  // namespace bison

#endif  // BISON_COMMON_CRASH_REPORTER_CRASH_KEYS_H_
