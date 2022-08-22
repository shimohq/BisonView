

#include "bison/browser/bv_render_process.h"

#include "bison/bison_jni_headers/BvRenderProcess_jni.h"

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "ipc/ipc_channel_proxy.h"

using base::android::AttachCurrentThread;
using content::BrowserThread;
using content::ChildProcessTerminationInfo;
using content::RenderProcessHost;

namespace bison {

const void* const kBisonRenderProcessKey = &kBisonRenderProcessKey;

// static
BvRenderProcess* BvRenderProcess::GetInstanceForRenderProcessHost(
    RenderProcessHost* host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  BvRenderProcess* render_process =
      static_cast<BvRenderProcess*>(host->GetUserData(kBisonRenderProcessKey));
  if (!render_process) {
    std::unique_ptr<BvRenderProcess> created_render_process =
        std::make_unique<BvRenderProcess>(host);
    render_process = created_render_process.get();
    host->SetUserData(kBisonRenderProcessKey, std::move(created_render_process));
  }
  return render_process;
}

BvRenderProcess::BvRenderProcess(RenderProcessHost* render_process_host)
    : render_process_host_(render_process_host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  java_obj_.Reset(Java_BvRenderProcess_create(AttachCurrentThread()));
  CHECK(!java_obj_.is_null());
  if (render_process_host_->IsReady()) {
    Ready();
  }
  render_process_host_->GetChannel()->GetRemoteAssociatedInterface(
      &renderer_remote_);
  render_process_host->AddObserver(this);
}

BvRenderProcess::~BvRenderProcess() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Java_BvRenderProcess_setNative(AttachCurrentThread(), java_obj_, 0);
  java_obj_.Reset();
}

void BvRenderProcess::ClearCache() {
  renderer_remote_->ClearCache();
}

void BvRenderProcess::SetJsOnlineProperty(bool network_up) {
  renderer_remote_->SetJsOnlineProperty(network_up);
}

void BvRenderProcess::Ready() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Java_BvRenderProcess_setNative(AttachCurrentThread(), java_obj_,
                                 reinterpret_cast<jlong>(this));
}

void BvRenderProcess::Cleanup() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  render_process_host_->RemoveObserver(this);
  render_process_host_->RemoveUserData(kBisonRenderProcessKey);
  // |this| is now deleted.
}

bool BvRenderProcess::TerminateChildProcess(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  return render_process_host_->Shutdown(0);
}

bool BvRenderProcess::IsProcessLockedToSiteForTesting(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  return render_process_host_->IsProcessLockedToSiteForTesting();  // IN-TEST
}

base::android::ScopedJavaLocalRef<jobject> BvRenderProcess::GetJavaObject() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  return base::android::ScopedJavaLocalRef<jobject>(java_obj_);
}

void BvRenderProcess::RenderProcessReady(RenderProcessHost* host) {
  DCHECK(host == render_process_host_);

  Ready();
}

void BvRenderProcess::RenderProcessExited(
    RenderProcessHost* host,
    const ChildProcessTerminationInfo& info) {
  DCHECK(host == render_process_host_);

  Cleanup();
}

}  // namespace bison
