package im.shimo.bison.internal;

import androidx.annotation.RestrictTo;
import androidx.annotation.NonNull;

import org.chromium.base.Callback;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

/**
 * Bridge between android.webview.WebStorage and native QuotaManager. This object is owned by Java
 * BvBrowserContext and the native side is owned by the native BvBrowserContext.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
@JNINamespace("bison")
public class BvQuotaManagerBridge {
    /**
     * This class represent the callback value of android.webview.WebStorage.getOrigins. The values
     * are optimized for JNI convenience and need to be converted.
     */
    @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
    public static class Origins {
        // Origin, usage, and quota data in parallel arrays of same length.
        public final String[] mOrigins;
        public final long[] mUsages;
        public final long[] mQuotas;

        Origins(String[] origins, long[] usages, long[] quotas) {
            mOrigins = origins;
            mUsages = usages;
            mQuotas = quotas;
        }
    }

    // This is not owning. The native object is owned by the native BvBrowserContext.
    private long mNativeBvQuotaManagerBridge;


    public BvQuotaManagerBridge(long nativeBvQuotaManagerBridge) {
        mNativeBvQuotaManagerBridge = nativeBvQuotaManagerBridge;
        BvQuotaManagerBridgeJni.get().init(nativeBvQuotaManagerBridge, this);
    }

    /*
     * There are four HTML5 offline storage APIs.
     * 1) Web Storage (ie the localStorage and sessionStorage variables)
     * 2) Web SQL database
     * 3) Indexed Database
     * 4) Filesystem API
     */

    /**
     * Implements WebStorage.deleteAllData(). Clear the storage of all five offline APIs.
     *
     * TODO(boliu): Actually clear Web Storage.
     */
    public void deleteAllData() {
        ThreadUtils.assertOnUiThread();
        BvQuotaManagerBridgeJni.get().deleteAllData(
                mNativeBvQuotaManagerBridge, BvQuotaManagerBridge.this);
    }

    /**
     * Implements WebStorage.deleteOrigin(). Clear the storage of APIs 2-5 for the given origin.
     */
    public void deleteOrigin(String origin) {
        ThreadUtils.assertOnUiThread();
        BvQuotaManagerBridgeJni.get().deleteOrigin(
                mNativeBvQuotaManagerBridge, this, origin);
    }

    /**
     * Implements WebStorage.getOrigins. Get the per origin usage and quota of APIs 2-5 in
     * aggregate.
     */
    public void getOrigins(@NonNull Callback<Origins> callback) {
        ThreadUtils.assertOnUiThread();
        BvQuotaManagerBridgeJni.get().getOrigins(
                mNativeBvQuotaManagerBridge, this, callback);
    }

    /**
     * Implements WebStorage.getQuotaForOrigin. Get the quota of APIs 2-5 in aggregate for given
     * origin.
     */
    public void getQuotaForOrigin(String origin, @NonNull Callback<Long> callback) {
        ThreadUtils.assertOnUiThread();
        BvQuotaManagerBridgeJni.get().getUsageAndQuotaForOrigin(
                mNativeBvQuotaManagerBridge, this, origin, callback, true);
    }

    /**
     * Implements WebStorage.getUsageForOrigin. Get the usage of APIs 2-5 in aggregate for given
     * origin.
     */
    public void getUsageForOrigin(String origin, @NonNull Callback<Long> callback) {
        ThreadUtils.assertOnUiThread();
        BvQuotaManagerBridgeJni.get().getUsageAndQuotaForOrigin(
                mNativeBvQuotaManagerBridge, this, origin, callback, false);
    }

    @CalledByNative
    private void onGetOriginsCallback(
            Callback<Origins> callback, String[] origin, long[] usages, long[] quotas) {
        callback.onResult(new Origins(origin, usages, quotas));
    }

    @NativeMethods
    interface Natives {
        void init(long nativeBvQuotaManagerBridge, BvQuotaManagerBridge caller);
        void deleteAllData(long nativeBvQuotaManagerBridge, BvQuotaManagerBridge caller);
        void deleteOrigin(
                long nativeBvQuotaManagerBridge, BvQuotaManagerBridge caller, String origin);
        void getOrigins(
                long nativeBvQuotaManagerBridge, BvQuotaManagerBridge caller, Callback<Origins> callback);
        void getUsageAndQuotaForOrigin(long nativeBvQuotaManagerBridge, BvQuotaManagerBridge caller,
                String origin, Callback<Long> callback, boolean isQuota);

    }
}
