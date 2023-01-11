

package im.shimo.bison.internal;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;

import androidx.annotation.VisibleForTesting;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

/**
 * Determines user consent and app opt-out for metrics. See bv_metrics_service_client.h for more
 * explanation.
 */
@JNINamespace("bison")
public class BvMetricsServiceClient {
    private static final String TAG = "BvMetricsServiceCli-";

    // Individual apps can use this meta-data tag in their manifest to opt out of metrics
    // reporting. See https://developer.android.com/reference/android/webkit/WebView.html
    private static final String OPT_OUT_META_DATA_STR = "android.webkit.WebView.MetricsOptOut";

    private static boolean isAppOptedOut(Context ctx) {
        try {
            ApplicationInfo info = ctx.getPackageManager().getApplicationInfo(
                    ctx.getPackageName(), PackageManager.GET_META_DATA);
            if (info.metaData == null) {
                // null means no such tag was found.
                return false;
            }
            // getBoolean returns false if the key is not found, which is what we want.
            return info.metaData.getBoolean(OPT_OUT_META_DATA_STR);
        } catch (PackageManager.NameNotFoundException e) {
            // This should never happen.
            Log.e(TAG, "App could not find itself by package name!");
            // The conservative thing is to assume the app HAS opted out.
            return true;
        }
    }

    /**
     * Set user consent settings.
     *
     * @param ctx application {@link Context}
     * @param userConsent user consent via Android Usage & diagnostics settings.
     * @return whether metrics reporting is enabled or not.
     */
    public static void setConsentSetting(Context ctx, boolean userConsent) {
        ThreadUtils.assertOnUiThread();
        BvMetricsServiceClientJni.get().setHaveMetricsConsent(userConsent, !isAppOptedOut(ctx));
    }

    @VisibleForTesting
    public static void setFastStartupForTesting(boolean fastStartupForTesting) {
        BvMetricsServiceClientJni.get().setFastStartupForTesting(fastStartupForTesting);
    }

    @VisibleForTesting
    public static void setUploadIntervalForTesting(long uploadIntervalMs) {
        BvMetricsServiceClientJni.get().setUploadIntervalForTesting(uploadIntervalMs);
    }

    /**
     * Sets a callback to run each time after final metrics have been collected.
     */
    @VisibleForTesting
    public static void setOnFinalMetricsCollectedListenerForTesting(Runnable listener) {
        BvMetricsServiceClientJni.get().setOnFinalMetricsCollectedListenerForTesting(listener);
        }

    @VisibleForTesting
    public static void setAppPackageNameLoggingRuleForTesting(String version, long expiryDateMs) {
        ThreadUtils.assertOnUiThread();
        BvMetricsServiceClientJni.get().setAppPackageNameLoggingRuleForTesting(
                version, expiryDateMs);
    }

    @NativeMethods
    interface Natives {
        void setHaveMetricsConsent(boolean userConsent, boolean appConsent);
        void setFastStartupForTesting(boolean fastStartupForTesting);
        void setUploadIntervalForTesting(long uploadIntervalMs);
        void setOnFinalMetricsCollectedListenerForTesting(Runnable listener);
        void setAppPackageNameLoggingRuleForTesting(String version, long expiryDateMs);
    }
}
