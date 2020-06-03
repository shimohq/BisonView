// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.core;

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

import im.shimo.bison.core.common.PlatformServiceBridge;

/**
 * Java side of the Browser Context: contains all the java side objects needed to host one
 * browsing session (i.e. profile).
 *
 * Note that historically WebView was running in single process mode, and limitations on renderer
 * process only being able to use a single browser context, currently there can only be one
 * BisonBrowserContext instance, so at this point the class mostly exists for conceptual clarity.
 */
@JNINamespace("bison")
public class BisonBrowserContext {
    private static final String CHROMIUM_PREFS_NAME = "WebViewProfilePrefsDefault";

    private static final String TAG = "BisonBrowserContext";
    private final SharedPreferences mSharedPreferences;

    private BisonGeolocationPermissions mGeolocationPermissions;
    private BisonFormDatabase mFormDatabase;
    private BisonServiceWorkerController mServiceWorkerController;
    private BisonQuotaManagerBridge mQuotaManagerBridge;

    /** Pointer to the Native-side BisonBrowserContext. */
    private long mNativeBisonBrowserContext;
    private final boolean mIsDefault;

    public BisonBrowserContext(
            SharedPreferences sharedPreferences, long nativeBisonBrowserContext, boolean isDefault) {
        mNativeBisonBrowserContext = nativeBisonBrowserContext;
        mSharedPreferences = sharedPreferences;

        mIsDefault = isDefault;
        if (isDefaultBisonBrowserContext()) {
            migrateGeolocationPreferences();
        }

        PlatformServiceBridge.getInstance().setSafeBrowsingHandler();

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

    public BisonServiceWorkerController getServiceWorkerController() {
        if (mServiceWorkerController == null) {
            mServiceWorkerController =
                    new BisonServiceWorkerController(ContextUtils.getApplicationContext(), this);
        }
        return mServiceWorkerController;
    }

    public BisonQuotaManagerBridge getQuotaManagerBridge() {
        if (mQuotaManagerBridge == null) {
            mQuotaManagerBridge = new BisonQuotaManagerBridge(
                    BisonBrowserContextJni.get().getQuotaManagerBridge(mNativeBisonBrowserContext));
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
            BisonGeolocationPermissions.migrateGeolocationPreferences(
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
        return mNativeBisonBrowserContext;
    }

    public boolean isDefaultBisonBrowserContext() {
        return mIsDefault;
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
