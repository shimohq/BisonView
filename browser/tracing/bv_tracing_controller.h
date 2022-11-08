#ifndef BISON_BROWSER_TRACING_AW_TRACING_CONTROLLER_H_
#define BISON_BROWSER_TRACING_AW_TRACING_CONTROLLER_H_

#include "base/android/jni_weak_ref.h"
#include "base/memory/weak_ptr.h"

namespace bison {

class BvTracingController {
 public:
  BvTracingController(JNIEnv* env, jobject obj);

  BvTracingController(const BvTracingController&) = delete;
  BvTracingController& operator=(const BvTracingController&) = delete;

  bool Start(JNIEnv* env,
             const base::android::JavaParamRef<jobject>& obj,
             const base::android::JavaParamRef<jstring>& categories,
             jint mode);
  bool StopAndFlush(JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& obj);
  bool IsTracing(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

 private:
  ~BvTracingController();

  void OnTraceDataReceived(std::unique_ptr<std::string> chunk);
  void OnTraceDataComplete();

  JavaObjectWeakGlobalRef weak_java_object_;
  base::WeakPtrFactory<BvTracingController> weak_factory_{this};
};

}  // namespace bison

#endif  // BISON_BROWSER_TRACING_AW_TRACING_CONTROLLER_H_
