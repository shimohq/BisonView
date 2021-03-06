

package im.shimo.bison.internal;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

/**
 * Determines user consent and app opt-out for metrics. See bison_metrics_service_client.h for more
 * explanation.
 */
@JNINamespace("bison")
public class BvMetricsServiceClient {
    private static final String TAG = "BisonMetricsServiceCli-";

    // Individual apps can use this meta-data tag in their manifest to opt out of metrics
    // reporting. See https://developer.android.com/reference/android/webkit/WebView.html
    private static final String OPT_OUT_META_DATA_STR = "android.webkit.WebView.MetricsOptOut";

    private static final String PLAY_STORE_PACKAGE_NAME = "com.android.vending";

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

    private static boolean shouldRecordPackageName(Context ctx) {
        // Only record if it's a system app or it was installed from Play Store.
        String packageName = ctx.getPackageName();
        String installerPackageName = ctx.getPackageManager().getInstallerPackageName(packageName);
        return (ctx.getApplicationInfo().flags & ApplicationInfo.FLAG_SYSTEM) != 0
                || (PLAY_STORE_PACKAGE_NAME.equals(installerPackageName));
    }

    public static void setConsentSetting(Context ctx, boolean userConsent) {
        ThreadUtils.assertOnUiThread();
        BvMetricsServiceClientJni.get().setHaveMetricsConsent(userConsent, !isAppOptedOut(ctx));
    }

    @CalledByNative
    private static String getAppPackageName() {
        Context ctx = ContextUtils.getApplicationContext();
        return shouldRecordPackageName(ctx) ? ctx.getPackageName() : null;
    }

    /**
     * Gets a long representing the install time of the embedder application. Units are in seconds,
     * as this is the resolution used by the metrics service. Returns {@code -1} upon failure.
     */
    // TODO(https://crbug.com/1012025): remove this when the kInstallDate pref has been persisted
    // for one or two milestones.
    @CalledByNative
    private static long getAppInstallTime() {
        try {
            Context ctx = ContextUtils.getApplicationContext();
            long installTimeMs = ctx.getPackageManager()
                                         .getPackageInfo(ctx.getPackageName(), 0 /* flags */)
                                         .firstInstallTime;
            long installTimeSec = installTimeMs / 1000;
            return installTimeSec;
        } catch (PackageManager.NameNotFoundException e) {
            // This should never happen.
            Log.e(TAG, "App could not find itself by package name!");
            return -1;
        }
    }

    @NativeMethods
    interface Natives {
        void setHaveMetricsConsent(boolean userConsent, boolean appConsent);
    }
}
