// create by jiang947 


#ifndef BISON_BROWSER_BISON_CONTENTS_LIFECYCLE_NOTIFIER_H_
#define BISON_BROWSER_BISON_CONTENTS_LIFECYCLE_NOTIFIER_H_

#include "base/android/jni_android.h"
#include "base/macros.h"

namespace bison {

class BisonContentsLifecycleNotifier {
 public:
  static void OnWebViewCreated();
  static void OnWebViewDestroyed();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BisonContentsLifecycleNotifier);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_CONTENTS_LIFECYCLE_NOTIFIER_H_
