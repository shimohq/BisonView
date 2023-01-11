package im.shimo.bison.internal;

import android.content.Context;
import android.content.SharedPreferences;
import androidx.annotation.RestrictTo;
import androidx.annotation.VisibleForTesting;
import im.shimo.bison.internal.BvContentsLifecycleNotifier.Observer;

import org.chromium.base.ContextUtils;
import org.chromium.base.StrictModeContext;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.memory.MemoryPressureMonitor;
import org.chromium.content_public.browser.ContentViewStatics;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
@JNINamespace("bison")
public class BvBrowserContext {
    private static final String CHROMIUM_PREFS_NAME = "BisonViewProfilePrefs";

    private static final String TAG = "BvBrowserContext";
    private final SharedPreferences mSharedPreferences;

    private BvGeolocationPermissions mGeolocationPermissions;
    private BvFormDatabase mFormDatabase;
    // jiang
    // private BisonServiceWorkerController mServiceWorkerController;
    private BvQuotaManagerBridge mQuotaManagerBridge;

    /** Pointer to the Native-side BvBrowserContext. */
    private long mNativeBvBrowserContext;
    private final boolean mIsDefault;

    public BvBrowserContext(
            SharedPreferences sharedPreferences, long nativeBvBrowserContext, boolean isDefault) {
        mNativeBvBrowserContext = nativeBvBrowserContext;
        mSharedPreferences = sharedPreferences;

        mIsDefault = isDefault;
        if (isDefaultBvBrowserContext()) {
            migrateGeolocationPreferences();
        }

        // Register MemoryPressureMonitor callbacks and make sure it polls only if there
        // is at
        // least one WebView around.
        MemoryPressureMonitor.INSTANCE.registerComponentCallbacks();
        BvContentsLifecycleNotifier.addObserver(new BvContentsLifecycleNotifier.Observer() {
            @Override
            public void onFirstWebViewCreated() {
                MemoryPressureMonitor.INSTANCE.enablePolling();
            }
            @Override
            public void onLastWebViewDestroyed() {
                MemoryPressureMonitor.INSTANCE.disablePolling();
            }
        });
    }

    @VisibleForTesting
    public void setNativePointer(long nativeBvBrowserContext) {
        mNativeBvBrowserContext = nativeBvBrowserContext;
    }

    public BvGeolocationPermissions getGeolocationPermissions() {
        if (mGeolocationPermissions == null) {
            mGeolocationPermissions = new BvGeolocationPermissions(mSharedPreferences);
        }
        return mGeolocationPermissions;
    }

    public BvFormDatabase getFormDatabase() {
        if (mFormDatabase == null) {
            mFormDatabase = new BvFormDatabase();
        }
        return mFormDatabase;
    }

    // public BisonServiceWorkerController getServiceWorkerController() {
    // if (mServiceWorkerController == null) {
    // mServiceWorkerController =
    // new BisonServiceWorkerController(ContextUtils.getApplicationContext(), this);
    // }
    // return mServiceWorkerController;
    // }

    public BvQuotaManagerBridge getQuotaManagerBridge() {
        if (mQuotaManagerBridge == null) {
            mQuotaManagerBridge = new BvQuotaManagerBridge(
                    BvBrowserContextJni.get().getQuotaManagerBridge(mNativeBvBrowserContext));
        }
        return mQuotaManagerBridge;
    }

    private void migrateGeolocationPreferences() {
        try (StrictModeContext ignored = StrictModeContext.allowDiskWrites()) {
            // Prefs dir will be created if it doesn't exist, so must allow writes
            // for this and so that the actual prefs can be written to the new
            // location if needed.
            final String oldGlobalPrefsName = "WebViewChromiumPrefs";
            SharedPreferences oldGlobalPrefs =
                    ContextUtils.getApplicationContext().getSharedPreferences(
                            oldGlobalPrefsName, Context.MODE_PRIVATE);
            BvGeolocationPermissions.migrateGeolocationPreferences(
                    oldGlobalPrefs, mSharedPreferences);
        }
    }

    /**
     * @see android.webkit.WebView#pauseTimers()
     */
    public void pauseTimers() {
        ContentViewStatics.setWebKitSharedTimersSuspended(true);
    }

    /**
     * @see android.webkit.WebView#resumeTimers()
     */
    public void resumeTimers() {
        ContentViewStatics.setWebKitSharedTimersSuspended(false);
    }

    public long getNativePointer() {
        return mNativeBvBrowserContext;
    }

    public boolean isDefaultBvBrowserContext() {
        return mIsDefault;
    }

    private static BvBrowserContext sInstance;

    public static BvBrowserContext getDefault() {
        if (sInstance == null) {
            sInstance = BvBrowserContextJni.get().getDefaultJava();
        }
        return sInstance;
    }

    @CalledByNative
    public static BvBrowserContext create(long nativeBvBrowserContext, boolean isDefault) {
        SharedPreferences sharedPreferences;
        try (StrictModeContext ignored = StrictModeContext.allowDiskWrites()) {
            // Prefs dir will be created if it doesn't exist, so must allow writes.
            sharedPreferences = ContextUtils.getApplicationContext().getSharedPreferences(
                    CHROMIUM_PREFS_NAME, Context.MODE_PRIVATE);
        }

        return new BvBrowserContext(sharedPreferences, nativeBvBrowserContext, isDefault);
    }

    // jiang947
    @CalledByNative
    public static boolean shouldSendVariationsHeaders() {
        // String packageId = PlatformServiceBridge.getInstance()
        // .getFirstPartyVariationsHeadersEnabledPackageId();
        // return !TextUtils.isEmpty(packageId)
        // && packageId.equals(ContextUtils.getApplicationContext().getPackageName());
        return false;
    }

    @NativeMethods
    interface Natives {
        BvBrowserContext getDefaultJava();

        long getQuotaManagerBridge(long nativeBvBrowserContext);
    }
}
