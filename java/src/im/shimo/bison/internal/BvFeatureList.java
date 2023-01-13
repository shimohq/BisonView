package im.shimo.bison.internal;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.build.annotations.MainDex;


@JNINamespace("bison")
@MainDex
public final class BvFeatureList {
    // Do not instantiate this class.
    private BvFeatureList() {}

    private static Boolean sPageStartedOnCommitForBrowserNavigations;

    private static boolean computePageStartedOnCommitForBrowserNavigations() {
        return true;
    }

    public static boolean pageStartedOnCommitEnabled(boolean isRendererInitiated) {
        // Always enable for renderer-initiated navigations.
        if (isRendererInitiated) return true;
        if (sPageStartedOnCommitForBrowserNavigations != null) {
            return sPageStartedOnCommitForBrowserNavigations;
        }
        sPageStartedOnCommitForBrowserNavigations =
                computePageStartedOnCommitForBrowserNavigations();
        return sPageStartedOnCommitForBrowserNavigations;
    }

    /**
     * Returns whether the specified feature is enabled or not.
     *
     * Note: Features queried through this API must be added to the array
     * |kFeaturesExposedToJava| in android_webview/browser/aw_feature_list.cc
     *
     * @param featureName The name of the feature to query.
     * @return Whether the feature is enabled or not.
     */
    public static boolean isEnabled(String featureName) {
        return BvFeatureListJni.get().isEnabled(featureName);
    }

    /**
     * Returns the configured feature parameter value as an integer.
     *
     * If the feature is not enabled or the parameter does not exist, this method
     * will return the |defaultValue|.
     *
     * Calling this method will mark the field trial as active. See details
     * in base/metrics/field_trial_params.h
     *
     * Note: Features queried through this API must be added to the array
     * |kFeaturesExposedToJava| in android_webview/browser/aw_feature_list.cc
     *
     * @param featureName The name of the feature to query.
     * @param paramName The name of the feature parameter to query.
     * @param defaultValue The default value to return if the feature or parameter is not found.
     * @return The configured parameter value as an integer.
     */
    public static int getFeatureParamValueAsInt(
            String featureName, String paramName, int defaultValue) {
        assert featureName != null : "featureName should not be null";
        assert paramName != null : "paramName should not be null";

        return BvFeatureListJni.get().getFeatureParamValueAsInt(
                featureName, paramName, defaultValue);
    }

    @NativeMethods
    interface Natives {
        boolean isEnabled(String featureName);
        int getFeatureParamValueAsInt(String featureName, String paramName, int defaultValue);
    }
}
