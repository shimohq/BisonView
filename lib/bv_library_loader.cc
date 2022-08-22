// create by jiang947

#include "bv_library_loader.h"

#include "base/android/jni_android.h"
#include "base/android/library_loader/library_loader_hooks.h"
#include "bv_main_delegate.h"
#include "components/version_info/version_info_values.h"
#include "content/public/app/content_jni_onload.h"
#include "content/public/app/content_main.h"


namespace bison {

  bool OnJNIOnLoadInit() {
  if (!content::android::OnJNIOnLoadInit())
    return false;

  //base::android::SetVersionNumber(PRODUCT_VERSION);
  content::SetContentMainDelegate(new bison::BvMainDelegate());
  return true;

}

} // namespace bison
