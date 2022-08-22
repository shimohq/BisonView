
#ifndef BISON_COMMON_BV_SWITCHES_H_
#define BISON_COMMON_BV_SWITCHES_H_


namespace switches {

extern const char kWebViewLogJsConsoleMessages[];
extern const char kWebViewSandboxedRenderer[];
extern const char kWebViewDisableSafebrowsingSupport[];
extern const char kWebViewSafebrowsingBlockAllResources[];
extern const char kHighlightAllWebViews[];
extern const char kWebViewVerboseLogging[];
extern const char kFinchSeedExpirationAge[];
extern const char kFinchSeedIgnorePendingDownload[];
extern const char kFinchSeedNoChargingRequirement[];
extern const char kFinchSeedMinDownloadPeriod[];
extern const char kFinchSeedMinUpdatePeriod[];
extern const char kWebViewEnableModernCookieSameSite[];
extern const char kWebViewDisableAppsPackageNamesAllowlistComponent[];
extern const char kWebViewDisablePackageAllowlistThrottling[];
extern const char kWebViewSelectiveImageInversionDarkening[];
extern const char kWebViewMPArchFencedFrames[];
extern const char kWebViewShadowDOMFencedFrames[];

}  // namespace switches

#endif  // BISON_COMMON_BV_SWITCHES_H_
