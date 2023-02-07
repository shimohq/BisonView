// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "bison/bison_jni_headers/BvFeatureList_jni.h"
#include "bison/common/bv_features.h"

#include "base/android/jni_string.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/notreached.h"
#include "content/public/common/content_features.h"
#include "third_party/blink/public/common/features.h"

using base::android::ConvertJavaStringToUTF8;
using base::android::JavaParamRef;

namespace bison {

namespace {

// Array of features exposed through the Java AwFeatureList API. Entries in
// this array may either refer to features defined in the header of this file or
// in other locations in the code base (e.g. content/, components/, etc).
const base::Feature* const kFeaturesExposedToJava[] = {
    &blink::features::kInitialNavigationEntry,
    &features::kWebViewConnectionlessSafeBrowsing,
    &features::kWebViewDisplayCutout,
    &features::kWebViewMixedContentAutoupgrades,
    &features::kWebViewTestFeature,
    &features::kWebViewMeasureScreenCoverage,
    &features::kWebViewJavaJsBridgeMojo,
    &features::kWebViewUseMetricsUploadService,
    &features::kWebViewXRequestedWithHeaderControl,
    &features::kWebViewXRequestedWithHeaderManifestAllowList,
    &features::kWebViewSynthesizePageLoadOnlyOnInitialMainDocumentAccess,
    &features::kWebViewClientHintsControllerDelegate,
};

const base::Feature* FindFeatureExposedToJava(const std::string& feature_name) {
  for (const base::Feature* feature : kFeaturesExposedToJava) {
    if (feature->name == feature_name)
      return feature;
  }
  NOTREACHED() << "Queried feature cannot be found in AwFeatureList: "
               << feature_name;
  return nullptr;
}

}  // namespace

static jboolean JNI_BvFeatureList_IsEnabled(
    JNIEnv* env,
    const JavaParamRef<jstring>& jfeature_name) {
  const base::Feature* feature =
      FindFeatureExposedToJava(ConvertJavaStringToUTF8(env, jfeature_name));
  return base::FeatureList::IsEnabled(*feature);
}

static jint JNI_BvFeatureList_GetFeatureParamValueAsInt(
    JNIEnv* env,
    const JavaParamRef<jstring>& jfeature_name,
    const JavaParamRef<jstring>& jparameter_name,
    const jint defaultValue) {
  const base::Feature* feature =
      FindFeatureExposedToJava(ConvertJavaStringToUTF8(env, jfeature_name));
  return base::GetFieldTrialParamByFeatureAsInt(
      *feature, ConvertJavaStringToUTF8(env, jparameter_name), defaultValue);
}

}  // namespace bison
