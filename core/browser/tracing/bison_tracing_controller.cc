// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/core/browser/tracing/bison_tracing_controller.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/tracing_controller.h"

#include "bison/core/browser_jni_headers/BisonTracingController_jni.h"

using base::android::JavaParamRef;

namespace {

base::android::ScopedJavaLocalRef<jbyteArray> StringToJavaBytes(
    JNIEnv* env,
    const std::string& str) {
  return base::android::ToJavaByteArray(
      env, reinterpret_cast<const uint8_t*>(str.data()), str.size());
}

class BisonTraceDataEndpoint
    : public content::TracingController::TraceDataEndpoint {
 public:
  using ReceivedChunkCallback =
      base::RepeatingCallback<void(std::unique_ptr<std::string>)>;

  static scoped_refptr<content::TracingController::TraceDataEndpoint> Create(
      ReceivedChunkCallback received_chunk_callback,
      base::OnceClosure completed_callback) {
    return new BisonTraceDataEndpoint(std::move(received_chunk_callback),
                                   std::move(completed_callback));
  }

  void ReceivedTraceFinalContents() override {
    base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                   std::move(completed_callback_));
  }

  void ReceiveTraceChunk(std::unique_ptr<std::string> chunk) override {
    base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                   base::BindOnce(received_chunk_callback_, std::move(chunk)));
  }

  explicit BisonTraceDataEndpoint(ReceivedChunkCallback received_chunk_callback,
                               base::OnceClosure completed_callback)
      : received_chunk_callback_(std::move(received_chunk_callback)),
        completed_callback_(std::move(completed_callback)) {}

 private:
  ~BisonTraceDataEndpoint() override {}

  ReceivedChunkCallback received_chunk_callback_;
  base::OnceClosure completed_callback_;

  DISALLOW_COPY_AND_ASSIGN(BisonTraceDataEndpoint);
};

}  // namespace

namespace bison {

static jlong JNI_BisonTracingController_Init(JNIEnv* env,
                                          const JavaParamRef<jobject>& obj) {
  BisonTracingController* controller = new BisonTracingController(env, obj);
  return reinterpret_cast<intptr_t>(controller);
}

BisonTracingController::BisonTracingController(JNIEnv* env, jobject obj)
    : weak_java_object_(env, obj) {}

BisonTracingController::~BisonTracingController() {}

bool BisonTracingController::Start(JNIEnv* env,
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

bool BisonTracingController::StopAndFlush(JNIEnv* env,
                                       const JavaParamRef<jobject>& obj) {
  // privacy_filtering_enabled=true is required for filtering out potential PII.
  return content::TracingController::GetInstance()->StopTracing(
      BisonTraceDataEndpoint::Create(
          base::BindRepeating(&BisonTracingController::OnTraceDataReceived,
                              weak_factory_.GetWeakPtr()),
          base::BindOnce(&BisonTracingController::OnTraceDataComplete,
                         weak_factory_.GetWeakPtr())),
      /*agent_label=*/"",
      /*privacy_filtering_enabled=*/true);
}

void BisonTracingController::OnTraceDataComplete() {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> obj = weak_java_object_.get(env);
  if (obj.obj()) {
    Java_BisonTracingController_onTraceDataComplete(env, obj);
  }
}

void BisonTracingController::OnTraceDataReceived(
    std::unique_ptr<std::string> chunk) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> obj = weak_java_object_.get(env);
  if (obj.obj()) {
    base::android::ScopedJavaLocalRef<jbyteArray> java_trace_data =
        StringToJavaBytes(env, *chunk);
    Java_BisonTracingController_onTraceDataChunkReceived(env, obj,
                                                      java_trace_data);
  }
}

bool BisonTracingController::IsTracing(JNIEnv* env,
                                    const JavaParamRef<jobject>& obj) {
  return content::TracingController::GetInstance()->IsTracing();
}

}  // namespace bison
