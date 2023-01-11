#include "bison/browser/tracing/bv_tracing_controller.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/trace_event/trace_config.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/tracing_controller.h"

#include "bison/bison_jni_headers/BvTracingController_jni.h"

using base::android::JavaParamRef;

namespace {

base::android::ScopedJavaLocalRef<jbyteArray> StringToJavaBytes(
    JNIEnv* env,
    const std::string& str) {
  return base::android::ToJavaByteArray(
      env, reinterpret_cast<const uint8_t*>(str.data()), str.size());
}

class BvTraceDataEndpoint
    : public content::TracingController::TraceDataEndpoint {
 public:
  using ReceivedChunkCallback =
      base::RepeatingCallback<void(std::unique_ptr<std::string>)>;

  static scoped_refptr<content::TracingController::TraceDataEndpoint> Create(
      ReceivedChunkCallback received_chunk_callback,
      base::OnceClosure completed_callback) {
    return new BvTraceDataEndpoint(std::move(received_chunk_callback),
                                   std::move(completed_callback));
  }

  void ReceivedTraceFinalContents() override {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, std::move(completed_callback_));
  }

  void ReceiveTraceChunk(std::unique_ptr<std::string> chunk) override {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(received_chunk_callback_, std::move(chunk)));
  }

  explicit BvTraceDataEndpoint(ReceivedChunkCallback received_chunk_callback,
                               base::OnceClosure completed_callback)
      : received_chunk_callback_(std::move(received_chunk_callback)),
        completed_callback_(std::move(completed_callback)) {}

  BvTraceDataEndpoint(const BvTraceDataEndpoint&) = delete;
  BvTraceDataEndpoint& operator=(const BvTraceDataEndpoint&) = delete;

 private:
  ~BvTraceDataEndpoint() override {}

  ReceivedChunkCallback received_chunk_callback_;
  base::OnceClosure completed_callback_;
};

}  // namespace

namespace bison {

static jlong JNI_BvTracingController_Init(JNIEnv* env,
                                          const JavaParamRef<jobject>& obj) {
  BvTracingController* controller = new BvTracingController(env, obj);
  return reinterpret_cast<intptr_t>(controller);
}

BvTracingController::BvTracingController(JNIEnv* env, jobject obj)
    : weak_java_object_(env, obj) {}

BvTracingController::~BvTracingController() {}

bool BvTracingController::Start(JNIEnv* env,
                                const JavaParamRef<jobject>& obj,
                                const JavaParamRef<jstring>& jcategories,
                                jint jmode) {
  std::string categories =
      base::android::ConvertJavaStringToUTF8(env, jcategories);
  base::trace_event::TraceConfig trace_config(
      categories, static_cast<base::trace_event::TraceRecordMode>(jmode));
  return content::TracingController::GetInstance()->StartTracing(
      trace_config, content::TracingController::StartTracingDoneCallback());
}

bool BvTracingController::StopAndFlush(JNIEnv* env,
                                       const JavaParamRef<jobject>& obj) {
  // privacy_filtering_enabled=true is required for filtering out potential PII.
  return content::TracingController::GetInstance()->StopTracing(
      BvTraceDataEndpoint::Create(
          base::BindRepeating(&BvTracingController::OnTraceDataReceived,
                              weak_factory_.GetWeakPtr()),
          base::BindOnce(&BvTracingController::OnTraceDataComplete,
                         weak_factory_.GetWeakPtr())),
      /*agent_label=*/"",
      /*privacy_filtering_enabled=*/true);
}

void BvTracingController::OnTraceDataComplete() {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> obj = weak_java_object_.get(env);
  if (obj.obj()) {
    Java_BvTracingController_onTraceDataComplete(env, obj);
  }
}

void BvTracingController::OnTraceDataReceived(
    std::unique_ptr<std::string> chunk) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> obj = weak_java_object_.get(env);
  if (obj.obj()) {
    base::android::ScopedJavaLocalRef<jbyteArray> java_trace_data =
        StringToJavaBytes(env, *chunk);
    Java_BvTracingController_onTraceDataChunkReceived(env, obj,
                                                      java_trace_data);
  }
}

bool BvTracingController::IsTracing(JNIEnv* env,
                                    const JavaParamRef<jobject>& obj) {
  return content::TracingController::GetInstance()->IsTracing();
}

}  // namespace bison
