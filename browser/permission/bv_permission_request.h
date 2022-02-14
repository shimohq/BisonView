// create by jiang947


#ifndef BISON_BROWSER_PERMISSION_BISON_PERMISSION_REQUEST_H_
#define BISON_BROWSER_PERMISSION_BISON_PERMISSION_REQUEST_H_

#include <stdint.h>

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "url/gurl.h"

namespace bison {

class BvPermissionRequestDelegate;

// This class wraps a permission request, it works with PermissionRequestHandler
// and its Java peer to represent the request to BvContentsClient.
// The specific permission request should implement the
// BvPermissionRequestDelegate interface, See MediaPermissionRequest.
// This object is owned by the java peer.
class BvPermissionRequest {
 public:
  // TODO jiang947 生成包名
  // GENERATED_JAVA_ENUM_PACKAGE: im.shimo.bison.internal
  enum PermissionResource {
    Geolocation = 1 << 0,
    VideoCapture = 1 << 1,
    AudioCapture = 1 << 2,
    ProtectedMediaId = 1 << 3,
    MIDISysex = 1 << 4,
  };

  // Take the ownership of |delegate|. Returns the native pointer in
  // |weak_ptr|, which is owned by the returned java peer.
  static base::android::ScopedJavaLocalRef<jobject> Create(
      std::unique_ptr<BvPermissionRequestDelegate> delegate,
      base::WeakPtr<BvPermissionRequest>* weak_ptr);

  // Return the Java peer. Must be null-checked.
  base::android::ScopedJavaLocalRef<jobject> GetJavaObject();

  // Invoked by Java peer when request is processed, |granted| indicates the
  // request was granted or not.
  void OnAccept(JNIEnv* env,
                const base::android::JavaParamRef<jobject>& jcaller,
                jboolean granted);
  void Destroy(JNIEnv* env);

  // Return the origin which initiated the request.
  const GURL& GetOrigin();

  // Return the resources origin requested.
  int64_t GetResources();

  // Cancel this request. Guarantee that
  // BvPermissionRequestDelegate::NotifyRequestResult will not be called after
  // this call. This also deletes this object, so weak pointers are invalidated
  // and raw pointers become dangling pointers.
  void CancelAndDelete();

 private:
  friend class TestPermissionRequestHandlerClient;

  BvPermissionRequest(std::unique_ptr<BvPermissionRequestDelegate> delegate,
                      base::android::ScopedJavaLocalRef<jobject>* java_peer);
  ~BvPermissionRequest();

  void OnAcceptInternal(bool accept);
  void DeleteThis();

  std::unique_ptr<BvPermissionRequestDelegate> delegate_;
  JavaObjectWeakGlobalRef java_ref_;

  bool processed_;
  base::WeakPtrFactory<BvPermissionRequest> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(BvPermissionRequest);
};

}  // namespace bison

#endif  // BISON_BROWSER_PERMISSION_BISON_PERMISSION_REQUEST_H_
