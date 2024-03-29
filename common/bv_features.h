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
extern const base::Feature kWebViewAppsPackageNamesAllowlist;
extern const base::FeatureParam<base::TimeDelta>
    kWebViewAppsMinAllowlistThrottleTimeDelta;
extern const base::FeatureParam<base::TimeDelta>
    kWebViewAppsMaxAllowlistThrottleTimeDelta;
extern const base::Feature kWebViewBrotliSupport;
extern const base::Feature kWebViewConnectionlessSafeBrowsing;
extern const base::Feature kWebViewDisplayCutout;
extern const base::Feature kWebViewEmptyComponentLoaderPolicy;
extern const base::Feature kWebViewExtraHeadersSameOriginOnly;
extern const base::Feature kWebViewForceDarkModeMatchTheme;
extern const base::Feature kWebViewJavaJsBridgeMojo;
extern const base::Feature kWebViewLegacyTlsSupport;
extern const base::Feature kWebViewMeasureScreenCoverage;
extern const base::Feature kWebViewMixedContentAutoupgrades;
extern const base::Feature kWebViewOriginTrials;
extern const base::Feature kWebViewSendVariationsHeaders;
extern const base::Feature kWebViewSuppressDifferentOriginSubframeJSDialogs;
extern const base::Feature kWebViewTestFeature;
extern const base::Feature kWebViewUseMetricsUploadService;
extern const base::Feature kWebViewWideColorGamutSupport;
extern const base::Feature kWebViewXRequestedWithHeader;
extern const base::FeatureParam<int> kWebViewXRequestedWithHeaderMode;
extern const base::Feature
    kWebViewSynthesizePageLoadOnlyOnInitialMainDocumentAccess;

}  // namespace features
}  // namespace bison

#endif  // BISON_COMMON_BV_FEATURES_H_
