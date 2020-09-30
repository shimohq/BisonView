// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/browser/bison_browser_context.h"
#include "bison/browser/bison_content_browser_client.h"
#include "bison/browser/bison_form_database_service.h"
#include "bison/bison_jni_headers/BisonFormDatabase_jni.h"

#include "base/android/jni_android.h"
#include "base/logging.h"
#include "base/time/time.h"

using base::android::JavaParamRef;

namespace bison {

namespace {

BisonFormDatabaseService* GetFormDatabaseService() {
  BisonBrowserContext* context = BisonBrowserContext::GetDefault();
  BisonFormDatabaseService* service = context->GetFormDatabaseService();
  return service;
}

}  // anonymous namespace

// static
jboolean JNI_BisonFormDatabase_HasFormData(JNIEnv*) {
  return GetFormDatabaseService()->HasFormData();
}

// static
void JNI_BisonFormDatabase_ClearFormData(JNIEnv*) {
  GetFormDatabaseService()->ClearFormData();
}

}  // namespace bison
