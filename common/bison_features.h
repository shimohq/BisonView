// create by jiang947

#ifndef BISON_COMMON_BISON_FEATURES_H_
#define BISON_COMMON_BISON_FEATURES_H_

#include "base/feature_list.h"

namespace bison {
namespace features {
extern const base::Feature kVizForWebView;
extern const base::Feature kWebViewBrotliSupport;
extern const base::Feature kWebViewConnectionlessSafeBrowsing;
extern const base::Feature kWebViewSniffMimeType;
extern const base::Feature kWebViewWideColorGamutSupport;

}  // namespace features

}  // namespace bison

#endif  // BISON_COMMON_BISON_FEATURES_H_