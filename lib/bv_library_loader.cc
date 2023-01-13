// create by jiang947

#include "bv_library_loader.h"
#include "bv_main_delegate.h"

#include "content/public/app/content_jni_onload.h"
#include "content/public/app/content_main.h"

namespace bison {

  bool OnJNIOnLoadInit() {
  if (!content::android::OnJNIOnLoadInit())
    return false;

  content::SetContentMainDelegate(new bison::BvMainDelegate());
  return true;
}

} // namespace bison
