// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.core;

import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Process;
import android.webkit.WebSettings;

import org.chromium.base.Log;
import org.chromium.base.annotations.JNINamespace;

/**
 * Stores Android WebView Service Worker specific settings.
 *
 * Methods in this class can be called from any thread, including threads created by
 * the client of WebView.
 */
@JNINamespace("bison")
public class BisonServiceWorkerSettings {
    private static final String LOGTAG = BisonServiceWorkerSettings.class.getSimpleName();
    private static final boolean TRACE = false;

    private int mCacheMode = WebSettings.LOAD_DEFAULT;
    private boolean mAllowContentUrlAccess = true;
    private boolean mAllowFileUrlAccess = true;
    private boolean mBlockNetworkLoads;  // Default depends on permission of the embedding APK
    private boolean mAcceptThirdPartyCookies;

    // Lock to protect all settings.
    private final Object mBisonServiceWorkerSettingsLock = new Object();

    // Computed on construction.
    private final boolean mHasInternetPermission;

    public BisonServiceWorkerSettings(Context context) {
        boolean hasInternetPermission = context.checkPermission(
                android.Manifest.permission.INTERNET,
                Process.myPid(),
                Process.myUid()) == PackageManager.PERMISSION_GRANTED;
        synchronized (mBisonServiceWorkerSettingsLock) {
            mHasInternetPermission = hasInternetPermission;
            mBlockNetworkLoads = !hasInternetPermission;
        }
    }

    /**
     * See {@link android.webkit.ServiceWorkerWebSettings#setCacheMode}.
     */
    public void setCacheMode(int mode) {
        if (TRACE) Log.d(LOGTAG, "setCacheMode=" + mode);
        synchronized (mBisonServiceWorkerSettingsLock) {
            if (mCacheMode != mode) {
                mCacheMode = mode;
            }
        }
    }

    /**
     * See {@link android.webkit.ServiceWorkerWebSettings#getCacheMode}.
     */
    public int getCacheMode() {
        synchronized (mBisonServiceWorkerSettingsLock) {
            return mCacheMode;
        }
    }

    /**
     * See {@link android.webkit.ServiceWorkerWebSettings#setAllowContentAccess}.
     */
    public void setAllowContentAccess(boolean allow) {
        if (TRACE) Log.d(LOGTAG, "setAllowContentAccess=" + allow);
        synchronized (mBisonServiceWorkerSettingsLock) {
            if (mAllowContentUrlAccess != allow) {
                mAllowContentUrlAccess = allow;
            }
        }
    }

    /**
     * See {@link android.webkit.ServiceWorkerWebSettings#getAllowContentAccess}.
     */
    public boolean getAllowContentAccess() {
        synchronized (mBisonServiceWorkerSettingsLock) {
            return mAllowContentUrlAccess;
        }
    }

    /**
     * See {@link android.webkit.ServiceWorkerWebSettings#setAllowFileAccess}.
     */
    public void setAllowFileAccess(boolean allow) {
        if (TRACE) Log.d(LOGTAG, "setAllowFileAccess=" + allow);
        synchronized (mBisonServiceWorkerSettingsLock) {
            if (mAllowFileUrlAccess != allow) {
                mAllowFileUrlAccess = allow;
            }
        }
    }

    /**
     * See {@link android.webkit.ServiceWorkerWebSettings#getAllowFileAccess}.
     */
    public boolean getAllowFileAccess() {
        synchronized (mBisonServiceWorkerSettingsLock) {
            return mAllowFileUrlAccess;
        }
    }

    /**
     * See {@link android.webkit.ServiceWorkerWebSettings#setBlockNetworkLoads}.
     */
    public void setBlockNetworkLoads(boolean flag) {
        if (TRACE) Log.d(LOGTAG, "setBlockNetworkLoads=" + flag);
        synchronized (mBisonServiceWorkerSettingsLock) {
            if (!flag && !mHasInternetPermission) {
                throw new SecurityException("Permission denied - "
                        + "application missing INTERNET permission");
            }
            mBlockNetworkLoads = flag;
        }
    }

    /**
     * See {@link android.webkit.ServiceWorkerWebSettings#getBlockNetworkLoads}.
     */
    public boolean getBlockNetworkLoads() {
        synchronized (mBisonServiceWorkerSettingsLock) {
            return mBlockNetworkLoads;
        }
    }
}
