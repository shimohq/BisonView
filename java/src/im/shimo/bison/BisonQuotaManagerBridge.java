package im.shimo.bison;

import android.util.SparseArray;

import org.chromium.base.Callback;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

/**
 * Bridge between android.webview.WebStorage and native QuotaManager. This object is owned by Java
 * BisonBrowserContext and the native side is owned by the native BisonBrowserContext.
 */
@JNINamespace("bison")
public class BisonQuotaManagerBridge {
    /**
     * This class represent the callback value of android.webview.WebStorage.getOrigins. The values
     * are optimized for JNI convenience and need to be converted.
     */
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

    // This is not owning. The native object is owned by the native BisonBrowserContext.
    private long mNativeBisonQuotaManagerBridge;

    // The Java callbacks are saved here. An incrementing callback id is generated for each saved
    // callback and is passed to the native side to identify callback.
    private int mNextId;
    private SparseArray<Callback<Origins>> mPendingGetOriginCallbacks;
    private SparseArray<Callback<Long>> mPendingGetQuotaForOriginCallbacks;
    private SparseArray<Callback<Long>> mPendingGetUsageForOriginCallbacks;

    public BisonQuotaManagerBridge(long nativeBisonQuotaManagerBridge) {
        mNativeBisonQuotaManagerBridge = nativeBisonQuotaManagerBridge;
        mPendingGetOriginCallbacks = new SparseArray<>();
        mPendingGetQuotaForOriginCallbacks = new SparseArray<>();
        mPendingGetUsageForOriginCallbacks = new SparseArray<>();
        BisonQuotaManagerBridgeJni.get().init(mNativeBisonQuotaManagerBridge, BisonQuotaManagerBridge.this);
    }

    private int getNextId() {
        ThreadUtils.assertOnUiThread();
        return ++mNextId;
    }

    /*
     * There are five HTML5 offline storage APIs.
     * 1) Web Storage (ie the localStorage and sessionStorage variables)
     * 2) Web SQL database
     * 3) Application cache
     * 4) Indexed Database
     * 5) Filesystem API
     */

    /**
     * Implements WebStorage.deleteAllData(). Clear the storage of all five offline APIs.
     *
     * TODO(boliu): Actually clear Web Storage.
     */
    public void deleteAllData() {
        BisonQuotaManagerBridgeJni.get().deleteAllData(
                mNativeBisonQuotaManagerBridge, BisonQuotaManagerBridge.this);
    }

    /**
     * Implements WebStorage.deleteOrigin(). Clear the storage of APIs 2-5 for the given origin.
     */
    public void deleteOrigin(String origin) {
        BisonQuotaManagerBridgeJni.get().deleteOrigin(
                mNativeBisonQuotaManagerBridge, BisonQuotaManagerBridge.this, origin);
    }

    /**
     * Implements WebStorage.getOrigins. Get the per origin usage and quota of APIs 2-5 in
     * aggregate.
     */
    public void getOrigins(Callback<Origins> callback) {
        int callbackId = getNextId();
        assert mPendingGetOriginCallbacks.get(callbackId) == null;
        mPendingGetOriginCallbacks.put(callbackId, callback);
        BisonQuotaManagerBridgeJni.get().getOrigins(
                mNativeBisonQuotaManagerBridge, BisonQuotaManagerBridge.this, callbackId);
    }

    /**
     * Implements WebStorage.getQuotaForOrigin. Get the quota of APIs 2-5 in aggregate for given
     * origin.
     */
    public void getQuotaForOrigin(String origin, Callback<Long> callback) {
        int callbackId = getNextId();
        assert mPendingGetQuotaForOriginCallbacks.get(callbackId) == null;
        mPendingGetQuotaForOriginCallbacks.put(callbackId, callback);
        BisonQuotaManagerBridgeJni.get().getUsageAndQuotaForOrigin(
                mNativeBisonQuotaManagerBridge, BisonQuotaManagerBridge.this, origin, callbackId, true);
    }

    /**
     * Implements WebStorage.getUsageForOrigin. Get the usage of APIs 2-5 in aggregate for given
     * origin.
     */
    public void getUsageForOrigin(String origin, Callback<Long> callback) {
        int callbackId = getNextId();
        assert mPendingGetUsageForOriginCallbacks.get(callbackId) == null;
        mPendingGetUsageForOriginCallbacks.put(callbackId, callback);
        BisonQuotaManagerBridgeJni.get().getUsageAndQuotaForOrigin(
                mNativeBisonQuotaManagerBridge, BisonQuotaManagerBridge.this, origin, callbackId, false);
    }

    @CalledByNative
    private void onGetOriginsCallback(int callbackId, String[] origin, long[] usages,
            long[] quotas) {
        assert mPendingGetOriginCallbacks.get(callbackId) != null;
        mPendingGetOriginCallbacks.get(callbackId).onResult(new Origins(origin, usages, quotas));
        mPendingGetOriginCallbacks.remove(callbackId);
    }

    @CalledByNative
    private void onGetUsageAndQuotaForOriginCallback(
            int callbackId, boolean isQuota, long usage, long quota) {
        if (isQuota) {
            assert mPendingGetQuotaForOriginCallbacks.get(callbackId) != null;
            mPendingGetQuotaForOriginCallbacks.get(callbackId).onResult(quota);
            mPendingGetQuotaForOriginCallbacks.remove(callbackId);
        } else {
            assert mPendingGetUsageForOriginCallbacks.get(callbackId) != null;
            mPendingGetUsageForOriginCallbacks.get(callbackId).onResult(usage);
            mPendingGetUsageForOriginCallbacks.remove(callbackId);
        }
    }

    @NativeMethods
    interface Natives {
        void init(long nativeBisonQuotaManagerBridge, BisonQuotaManagerBridge caller);
        void deleteAllData(long nativeBisonQuotaManagerBridge, BisonQuotaManagerBridge caller);
        void deleteOrigin(
                long nativeBisonQuotaManagerBridge, BisonQuotaManagerBridge caller, String origin);
        void getOrigins(
                long nativeBisonQuotaManagerBridge, BisonQuotaManagerBridge caller, int callbackId);
        void getUsageAndQuotaForOrigin(long nativeBisonQuotaManagerBridge, BisonQuotaManagerBridge caller,
                String origin, int callbackId, boolean isQuota);

    }
}
