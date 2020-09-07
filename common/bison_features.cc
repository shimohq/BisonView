#include "bison_features.h"

namespace bison {
namespace features {

// Alphabetical:

// Viz for WebView architecture.
const base::Feature kVizForWebView{"VizForWebView",
                                   base::FEATURE_DISABLED_BY_DEFAULT};

// Enable brotli compression support in WebView.
const base::Feature kWebViewBrotliSupport{"WebViewBrotliSupport",
                                          base::FEATURE_DISABLED_BY_DEFAULT};

// Use the SafeBrowsingApiHandler which uses the connectionless GMS APIs. This
// Feature is checked and used in downstream internal code.
const base::Feature kWebViewConnectionlessSafeBrowsing{
    "WebViewConnectionlessSafeBrowsing", base::FEATURE_DISABLED_BY_DEFAULT};

// Sniff the content stream to guess the MIME type when the application doesn't
// tell us the MIME type explicitly.
//
// This only applies:
// * when NetworkService is enabled (if disabled, the legacy net path sniffs
//   content anyway, as an implementation detail).
// * to app-provided content (shouldInterceptRequest,
//   file:///android_{asset,res} URLs, content:// URLs), rather than content
//   from the net stack (we may sniff content from the net stack anyway,
//   depending on headers, but that's a NetworkService implementation detail).
const base::Feature kWebViewSniffMimeType{"WebViewSniffMimeType",
                                          base::FEATURE_ENABLED_BY_DEFAULT};

// Enable raster in wide color gamut for apps that use webview in a wide color
// gamut activity.
const base::Feature kWebViewWideColorGamutSupport{
    "WebViewWideColorGamutSupport", base::FEATURE_ENABLED_BY_DEFAULT};

}  // namespace features

}  // namespace bison