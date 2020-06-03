// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_BROWSER_PERMISSION_BISON_PERMISSION_REQUEST_H_
#define BISON_CORE_BROWSER_PERMISSION_BISON_PERMISSION_REQUEST_H_

#include <stdint.h>

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "url/gurl.h"

namespace bison {

class BisonPermissionRequestDelegate;

// This class wraps a permission request, it works with PermissionRequestHandler
// and its Java peer to represent the request to BisonContentsClient.
// The specific permission request should implement the
// BisonPermissionRequestDelegate interface, See MediaPermissionRequest.
// This object is owned by the java peer.
class BisonPermissionRequest {
 public:
  // TODO jiang947 这里的包名是否改成自己的 需要考虑下
  // GENERATED_JAVA_ENUM_PACKAGE: org.chromium.bison.permission
  enum Resource {
    Geolocation = 1 << 0,
    VideoCapture = 1 << 1,
    AudioCapture = 1 << 2,
    ProtectedMediaId = 1 << 3,
    MIDISysex = 1 << 4,
  };

  // Take the ownership of |delegate|. Returns the native pointer in
  // |weak_ptr|, which is owned by the returned java peer.
  static base::android::ScopedJavaLocalRef<jobject> Create(
      std::unique_ptr<BisonPermissionRequestDelegate> delegate,
      base::WeakPtr<BisonPermissionRequest>* weak_ptr);

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
  // BisonPermissionRequestDelegate::NotifyRequestResult will not be called after
  // this call. This also deletes this object, so weak pointers are invalidated
  // and raw pointers become dangling pointers.
  void CancelAndDelete();

 private:
  friend class TestPermissionRequestHandlerClient;

  BisonPermissionRequest(std::unique_ptr<BisonPermissionRequestDelegate> delegate,
                      base::android::ScopedJavaLocalRef<jobject>* java_peer);
  ~BisonPermissionRequest();

  void OnAcceptInternal(bool accept);
  void DeleteThis();

  std::unique_ptr<BisonPermissionRequestDelegate> delegate_;
  JavaObjectWeakGlobalRef java_ref_;

  bool processed_;
  base::WeakPtrFactory<BisonPermissionRequest> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(BisonPermissionRequest);
};

}  // namespace bison

#endif  // BISON_CORE_BROWSER_PERMISSION_BISON_PERMISSION_REQUEST_H_
