// create by jiang947

#include "base/android/jni_android.h"
#include "bison_main_delegate.h"
#include "content/public/app/content_jni_onload.h"
#include "content/public/app/content_main.h"

JNI_EXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  base::android::InitVM(vm);
  if (!content::android::OnJNIOnLoadInit())
    return -1;
  content::SetContentMainDelegate(new bison::BisonMainDelegate());
  return JNI_VERSION_1_4;
}
