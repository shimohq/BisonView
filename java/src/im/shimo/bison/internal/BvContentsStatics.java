package im.shimo.bison.internal;

import android.content.Context;
import android.net.Uri;

// import org.chromium.android_webview.common.Flag;
// import org.chromium.android_webview.common.FlagOverrideHelper;
// import org.chromium.android_webview.common.PlatformServiceBridge;
// import org.chromium.android_webview.common.ProductionSupportedFlagList;
import org.chromium.base.Callback;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.content_public.browser.UiThreadTaskTraits;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Implementations of various static methods, and also a home for static
 * data structures that are meant to be shared between all webviews.
 */
@JNINamespace("bison")
public class BvContentsStatics {

    private static ClientCertLookupTable sClientCertLookupTable;

    private static String sUnreachableWebDataUrl;

    private static boolean sRecordFullDocument;

    /**
     * Return the client certificate lookup table.
     */
    public static ClientCertLookupTable getClientCertLookupTable() {
        ThreadUtils.assertOnUiThread();
        if (sClientCertLookupTable == null) {
            sClientCertLookupTable = new ClientCertLookupTable();
        }
        return sClientCertLookupTable;
    }

    /**
     * Clear client cert lookup table. Should only be called from UI thread.
     */
    public static void clearClientCertPreferences(Runnable callback) {
        ThreadUtils.assertOnUiThread();
        getClientCertLookupTable().clear();
        BvContentsStaticsJni.get().clearClientCertPreferences(callback);
    }

    @CalledByNative
    private static void clientCertificatesCleared(Runnable callback) {
        if (callback == null) return;
        callback.run();
    }

    public static String getUnreachableWebDataUrl() {
        // Note that this method may be called from both IO and UI threads,
        // but as it only retrieves a value of a constant from native, even if
        // two calls will be running at the same time, this should not cause
        // any harm.
        if (sUnreachableWebDataUrl == null) {
            sUnreachableWebDataUrl = BvContentsStaticsJni.get().getUnreachableWebDataUrl();
        }
        return sUnreachableWebDataUrl;
    }

    public static void setRecordFullDocument(boolean recordFullDocument) {
        sRecordFullDocument = recordFullDocument;
    }

    /* package */ static boolean getRecordFullDocument() {
        return sRecordFullDocument;
    }

    public static String getProductVersion() {
        return BvContentsStaticsJni.get().getProductVersion();
    }

    public static void setServiceWorkerIoThreadClient(
            BvContentsIoThreadClient ioThreadClient, BvBrowserContext browserContext) {
        BvContentsStaticsJni.get().setServiceWorkerIoThreadClient(ioThreadClient, browserContext);
    }

    // @CalledByNative
    // private static void safeBrowsingAllowlistAssigned(Callback<Boolean> callback, boolean success) {
    //     if (callback == null) return;
    //     callback.onResult(success);
    // }

    // public static void setSafeBrowsingAllowlist(List<String> urls, Callback<Boolean> callback) {
    //     String[] urlArray = urls.toArray(new String[urls.size()]);
    //     if (callback == null) {
    //         callback = b -> {
    //         };
    //     }
    //     BvContentsStaticsJni.get().setSafeBrowsingAllowlist(urlArray, callback);
    // }

    // public static Uri getSafeBrowsingPrivacyPolicyUrl() {
    //     return Uri.parse(BvContentsStaticsJni.get().getSafeBrowsingPrivacyPolicyUrl());
    // }

    public static void setCheckClearTextPermitted(boolean permitted) {
        BvContentsStaticsJni.get().setCheckClearTextPermitted(permitted);
    }

    public static void logCommandLineForDebugging() {
        BvContentsStaticsJni.get().logCommandLineForDebugging();
    }

    public static void logFlagOverridesWithNative(Map<String, Boolean> flagOverrides) {
        // Do work asynchronously to avoid blocking startup.
        // PostTask.postTask(TaskTraits.THREAD_POOL_BEST_EFFORT, () -> {
        //     FlagOverrideHelper helper =
        //             new FlagOverrideHelper(ProductionSupportedFlagList.sFlagList);
        //     ArrayList<String> switches = new ArrayList<>();
        //     ArrayList<String> features = new ArrayList<>();
        //     for (Map.Entry<String, Boolean> entry : flagOverrides.entrySet()) {
        //         Flag flag = helper.getFlagForName(entry.getKey());
        //         boolean enabled = entry.getValue();
        //         if (flag.isBaseFeature()) {
        //             features.add(flag.getName() + (enabled ? ":enabled" : ":disabled"));
        //         } else if (enabled) {
        //             switches.add("--" + flag.getName());
        //         }
        //         // Only insert enabled switches; ignore explicitly disabled switches since this is
        //         // usually a NOOP.
        //     }
        //     BvContentsStaticsJni.get().logFlagMetrics(
        //             switches.toArray(new String[0]), features.toArray(new String[0]));
        // });
    }

    /**
     * Returns true if WebView is running in multi process mode.
     */
    public static boolean isMultiProcessEnabled() {
        return BvContentsStaticsJni.get().isMultiProcessEnabled();
    }

    /**
     * Returns the variations header used with the X-Client-Data header.
     */
    public static String getVariationsHeader() {
        return BvContentsStaticsJni.get().getVariationsHeader();
    }

    @NativeMethods
    interface Natives {
        void logCommandLineForDebugging();
        void logFlagMetrics(String[] switches, String[] features);

        //String getSafeBrowsingPrivacyPolicyUrl();
        void clearClientCertPreferences(Runnable callback);
        String getUnreachableWebDataUrl();
        String getProductVersion();
        void setServiceWorkerIoThreadClient(
                BvContentsIoThreadClient ioThreadClient, BvBrowserContext browserContext);
        //void setSafeBrowsingAllowlist(String[] urls, Callback<Boolean> callback);
        void setCheckClearTextPermitted(boolean permitted);
        boolean isMultiProcessEnabled();
        String getVariationsHeader();
    }
}
