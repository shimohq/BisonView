// create by jiang947

#ifndef BISON_COMMON_BV_FEATURES_H_
#define BISON_COMMON_BV_FEATURES_H_


#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"

namespace bison {
namespace features {

// All features in alphabetical order. The features should be documented
// alongside the definition of their values in the .cc file.

// Alphabetical:
BASE_DECLARE_FEATURE(kWebViewBrotliSupport);
BASE_DECLARE_FEATURE(kWebViewCheckReturnResources);
BASE_DECLARE_FEATURE(kWebViewConnectionlessSafeBrowsing);
BASE_DECLARE_FEATURE(kWebViewDisplayCutout);
BASE_DECLARE_FEATURE(kWebViewEmptyComponentLoaderPolicy);
BASE_DECLARE_FEATURE(kWebViewExtraHeadersSameOriginOnly);
BASE_DECLARE_FEATURE(kWebViewForceDarkModeMatchTheme);
BASE_DECLARE_FEATURE(kWebViewHitTestInBlinkOnTouchStart);
BASE_DECLARE_FEATURE(kWebViewJavaJsBridgeMojo);
BASE_DECLARE_FEATURE(kWebViewLegacyTlsSupport);
BASE_DECLARE_FEATURE(kWebViewMeasureScreenCoverage);
BASE_DECLARE_FEATURE(kWebViewMixedContentAutoupgrades);
BASE_DECLARE_FEATURE(kWebViewOriginTrials);
BASE_DECLARE_FEATURE(kWebViewRecordAppDataDirectorySize);
BASE_DECLARE_FEATURE(kWebViewSuppressDifferentOriginSubframeJSDialogs);
BASE_DECLARE_FEATURE(kWebViewSynthesizePageLoadOnlyOnInitialMainDocumentAccess);
BASE_DECLARE_FEATURE(kWebViewTestFeature);
BASE_DECLARE_FEATURE(kWebViewUseMetricsUploadService);
BASE_DECLARE_FEATURE(kWebViewWideColorGamutSupport);
BASE_DECLARE_FEATURE(kWebViewXRequestedWithHeaderControl);
extern const base::FeatureParam<int> kWebViewXRequestedWithHeaderMode;
BASE_DECLARE_FEATURE(kWebViewXRequestedWithHeaderManifestAllowList);
BASE_DECLARE_FEATURE(kWebViewClientHintsControllerDelegate);

}  // namespace features
}  // namespace bison

#endif  // BISON_COMMON_BV_FEATURES_H_
