// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <jni.h>
#include <string>

#include "bison/browser/variations/variations_seed_loader.h"

#include "bison/bison_jni_headers/VariationsSeedLoader_jni.h"
#include "bison/proto/bv_variations_seed.pb.h"

#include "base/android/jni_string.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"

using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;

namespace bison {

BvVariationsSeed* g_seed = nullptr;

static jboolean JNI_VariationsSeedLoader_ParseAndSaveSeedProto(
    JNIEnv* env,
    const JavaParamRef<jstring>& seed_path) {
  // Parse the proto.
  std::unique_ptr<BvVariationsSeed> seed =
      std::make_unique<BvVariationsSeed>(BvVariationsSeed::default_instance());
  std::string native_seed_path = ConvertJavaStringToUTF8(seed_path);
  base::ScopedFD seed_fd(open(native_seed_path.c_str(), O_RDONLY));
  if (!seed->ParseFromFileDescriptor(seed_fd.get())) {
    return false;
  }

  // Empty or incomplete protos should be considered invalid. An empty seed
  // file is expected when we request a seed from the service, but no new seed
  // is available. In that case, an empty seed file will have been created, but
  // never written to.
  if (!seed->has_signature() || !seed->has_date() || !seed->has_country() ||
      !seed->has_is_gzip_compressed() || !seed->has_seed_data()) {
    return false;
  }
  g_seed = seed.release();
  return true;
}

static jlong JNI_VariationsSeedLoader_GetSavedSeedDate(JNIEnv* env) {
  return g_seed ? g_seed->date() : 0;
}

std::unique_ptr<BvVariationsSeed> TakeSeed() {
  std::unique_ptr<BvVariationsSeed> seed(g_seed);
  g_seed = nullptr;
  return seed;
}

}  // namespace bison
