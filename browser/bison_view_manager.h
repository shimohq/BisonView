// create by jiang947

#ifndef BISON_BROWSER_BISON_VIEW_MANAGER_H_
#define BISON_BROWSER_BISON_VIEW_MANAGER_H_

#include <jni.h>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"

class BisonView;

namespace cc {
class Layer;
}

namespace bison {

base::android::ScopedJavaLocalRef<jobject> CreateShellView(
    BisonView* bison_view);

void RemoveShellView(const base::android::JavaRef<jobject>& shell_view);

void ShellAttachLayer(cc::Layer* layer);
void ShellRemoveLayer(cc::Layer* layer);

void DestroyShellManager();

}  // namespace bison

#endif  // BISON_BROWSER_BISON_VIEW_MANAGER_H_
