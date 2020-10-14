package im.shimo.bison;

import android.content.Context;
import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;
import org.chromium.base.StrictModeContext;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.memory.MemoryPressureMonitor;
import org.chromium.content_public.browser.ContentViewStatics;


@JNINamespace("bison")
public class BisonBrowserContext {
    private static final String CHROMIUM_PREFS_NAME = "BisonViewProfilePrefs";

    private static final String TAG = "BisonBrowserContext";
    private final SharedPreferences mSharedPreferences;

    private BisonGeolocationPermissions mGeolocationPermissions;
    
    private BisonFormDatabase mFormDatabase;
    // jiang 
    // private BisonServiceWorkerController mServiceWorkerController;
    private BisonQuotaManagerBridge mQuotaManagerBridge;

    /** Pointer to the Native-side BisonBrowserContext. */
    private long mNativeBisonBrowserContext;
    private final boolean mIsDefault;

    public BisonBrowserContext(
            SharedPreferences sharedPreferences, long nativeBisonBrowserContext, boolean isDefault) {
        mNativeBisonBrowserContext = nativeBisonBrowserContext;
        mSharedPreferences = sharedPreferences;

        mIsDefault = isDefault;

        // Register MemoryPressureMonitor callbacks and make sure it polls only if there is at
        // least one WebView around.
        MemoryPressureMonitor.INSTANCE.registerComponentCallbacks();
        BisonContentsLifecycleNotifier.addObserver(new BisonContentsLifecycleNotifier.Observer() {
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
    public void setNativePointer(long nativeBisonBrowserContext) {
        mNativeBisonBrowserContext = nativeBisonBrowserContext;
    }

    public BisonGeolocationPermissions getGeolocationPermissions() {
        if (mGeolocationPermissions == null) {
            mGeolocationPermissions = new BisonGeolocationPermissions(mSharedPreferences);
        }
        return mGeolocationPermissions;
    }

    public BisonFormDatabase getFormDatabase() {
        if (mFormDatabase == null) {
            mFormDatabase = new BisonFormDatabase();
        }
        return mFormDatabase;
    }

    // public BisonServiceWorkerController getServiceWorkerController() {
    //     if (mServiceWorkerController == null) {
    //         mServiceWorkerController =
    //                 new BisonServiceWorkerController(ContextUtils.getApplicationContext(), this);
    //     }
    //     return mServiceWorkerController;
    // }

    public BisonQuotaManagerBridge getQuotaManagerBridge() {
        if (mQuotaManagerBridge == null) {
            mQuotaManagerBridge = new BisonQuotaManagerBridge(
                    BisonBrowserContextJni.get().getQuotaManagerBridge(mNativeBisonBrowserContext));
        }
        return mQuotaManagerBridge;
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
        return mNativeBisonBrowserContext;
    }

    

    private static BisonBrowserContext sInstance;
    public static BisonBrowserContext getDefault() {
        if (sInstance == null) {
            sInstance = BisonBrowserContextJni.get().getDefaultJava();
        }
        return sInstance;
    }

    @CalledByNative
    public static BisonBrowserContext create(long nativeBisonBrowserContext, boolean isDefault) {
        SharedPreferences sharedPreferences;
        try (StrictModeContext ignored = StrictModeContext.allowDiskWrites()) {
            // Prefs dir will be created if it doesn't exist, so must allow writes.
            sharedPreferences = ContextUtils.getApplicationContext().getSharedPreferences(
                    CHROMIUM_PREFS_NAME, Context.MODE_PRIVATE);
        }

        return new BisonBrowserContext(sharedPreferences, nativeBisonBrowserContext, isDefault);
    }

    @NativeMethods
    interface Natives {
        BisonBrowserContext getDefaultJava();
        long getQuotaManagerBridge(long nativeBisonBrowserContext);
    }
}
