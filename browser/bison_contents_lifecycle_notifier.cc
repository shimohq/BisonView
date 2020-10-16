#include "bison/browser/bison_contents_lifecycle_notifier.h"

#include "bison/bison_jni_headers/BisonContentsLifecycleNotifier_jni.h"

using base::android::AttachCurrentThread;

namespace bison {

// static
void BisonContentsLifecycleNotifier::OnWebViewCreated() {
  Java_BisonContentsLifecycleNotifier_onWebViewCreated(AttachCurrentThread());
}

// static
void BisonContentsLifecycleNotifier::OnWebViewDestroyed() {
  Java_BisonContentsLifecycleNotifier_onWebViewDestroyed(AttachCurrentThread());
}

}  // namespace bison
