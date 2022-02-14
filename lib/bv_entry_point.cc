#include "bison/lib/bv_library_loader.h"

#include "base/android/jni_android.h"
#include "base/android/library_loader/library_loader_hooks.h"

namespace {

bool NativeInit(base::android::LibraryProcessType library_process_type) {
    return bison::OnJNIOnLoadInit();
}

}  // namespace

JNI_EXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  base::android::InitVM(vm);
  base::android::SetNativeInitializationHook(&NativeInit);
  return JNI_VERSION_1_4;
}
