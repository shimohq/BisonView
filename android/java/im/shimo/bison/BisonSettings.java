// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.os.Process;
import android.provider.Settings;
import android.util.Log;
import android.webkit.WebSettings;

import androidx.annotation.IntDef;

import im.shimo.bison.safe_browsing.BisonSafeBrowsingConfigHelper;
import im.shimo.bison.settings.ForceDarkBehavior;
import im.shimo.bison.settings.ForceDarkMode;
import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.content_public.browser.WebContents;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Stores Android WebView specific settings that does not need to be synced to WebKit.
 *
 * Methods in this class can be called from any thread, including threads created by
 * the client of WebView.
 */
@JNINamespace("bison")
public class BisonSettings {
    private static final String LOGTAG = BisonSettings.class.getSimpleName();
    private static final boolean TRACE = false;

    private static final String TAG = "BisonSettings";

    /* See {@link android.webkit.WebSettings}. */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({LAYOUT_ALGORITHM_NORMAL,
            /* See {@link android.webkit.WebSettings}. */
            LAYOUT_ALGORITHM_SINGLE_COLUMN,
            /* See {@link android.webkit.WebSettings}. */
            LAYOUT_ALGORITHM_NARROW_COLUMNS, LAYOUT_ALGORITHM_TEXT_AUTOSIZING})
    public @interface LayoutAlgorithm {}
    public static final int LAYOUT_ALGORITHM_NORMAL = 0;
    /* See {@link android.webkit.WebSettings}. */
    public static final int LAYOUT_ALGORITHM_SINGLE_COLUMN = 1;
    /* See {@link android.webkit.WebSettings}. */
    public static final int LAYOUT_ALGORITHM_NARROW_COLUMNS = 2;
    public static final int LAYOUT_ALGORITHM_TEXT_AUTOSIZING = 3;

    public static final int FORCE_DARK_OFF = ForceDarkMode.FORCE_DARK_OFF;
    public static final int FORCE_DARK_AUTO = ForceDarkMode.FORCE_DARK_AUTO;
    public static final int FORCE_DARK_ON = ForceDarkMode.FORCE_DARK_ON;

    @ForceDarkMode
    private int mForceDarkMode = ForceDarkMode.FORCE_DARK_AUTO;

    public static final int FORCE_DARK_ONLY = ForceDarkBehavior.FORCE_DARK_ONLY;
    public static final int MEDIA_QUERY_ONLY = ForceDarkBehavior.MEDIA_QUERY_ONLY;
    // This option requires RuntimeEnabledFeatures::MetaColorSchemeEnabled()
    public static final int PREFER_MEDIA_QUERY_OVER_FORCE_DARK =
            ForceDarkBehavior.PREFER_MEDIA_QUERY_OVER_FORCE_DARK;

    @ForceDarkBehavior
    private int mForceDarkBehavior = ForceDarkBehavior.PREFER_MEDIA_QUERY_OVER_FORCE_DARK;

    // This class must be created on the UI thread. Afterwards, it can be
    // used from any thread. Internally, the class uses a message queue
    // to call native code on the UI thread only.

    // Values passed in on construction.
    private final boolean mHasInternetPermission;

    private ZoomSupportChangeListener mZoomChangeListener;
    private double mDIPScale = 1.0;

    // Lock to protect all settings.
    private final Object mBisonSettingsLock = new Object();

    @LayoutAlgorithm
    private int mLayoutAlgorithm = LAYOUT_ALGORITHM_NARROW_COLUMNS;
    private int mTextSizePercent = 100;
    private String mStandardFontFamily = "sans-serif";
    private String mFixedFontFamily = "monospace";
    private String mSansSerifFontFamily = "sans-serif";
    private String mSerifFontFamily = "serif";
    private String mCursiveFontFamily = "cursive";
    private String mFantasyFontFamily = "fantasy";
    private String mDefaultTextEncoding = "UTF-8";
    private String mUserAgent;
    private int mMinimumFontSize = 8;
    private int mMinimumLogicalFontSize = 8;
    private int mDefaultFontSize = 16;
    private int mDefaultFixedFontSize = 13;
    private boolean mLoadsImagesAutomatically = true;
    private boolean mImagesEnabled = true;
    private boolean mJavaScriptEnabled;
    private boolean mAllowUniversalAccessFromFileURLs;
    private boolean mAllowFileAccessFromFileURLs;
    private boolean mJavaScriptCanOpenWindowsAutomatically;
    private boolean mSupportMultipleWindows;
    private boolean mAppCacheEnabled;
    private boolean mDomStorageEnabled;
    private boolean mDatabaseEnabled;
    private boolean mUseWideViewport;
    private boolean mZeroLayoutHeightDisablesViewportQuirk;
    private boolean mForceZeroLayoutHeight;
    private boolean mLoadWithOverviewMode;
    private boolean mMediaPlaybackRequiresUserGesture = true;
    private String mDefaultVideoPosterURL;
    private float mInitialPageScalePercent;
    private boolean mSpatialNavigationEnabled;  // Default depends on device features.
    private boolean mEnableSupportedHardwareAcceleratedFeatures;
    private int mMixedContentMode = WebSettings.MIXED_CONTENT_NEVER_ALLOW;
    private boolean mCSSHexAlphaColorEnabled;
    private boolean mScrollTopLeftInteropEnabled;
    private boolean mWillSuppressErrorPage;

    private boolean mOffscreenPreRaster;
    private int mDisabledMenuItems = WebSettings.MENU_ITEM_NONE;

    // Although this bit is stored on BisonSettings it is actually controlled via the CookieManager.
    private boolean mAcceptThirdPartyCookies;

    // if null, default to BisonSafeBrowsingConfigHelper.getSafeBrowsingEnabledByManifest()
    private Boolean mSafeBrowsingEnabled;

    private final boolean mSupportLegacyQuirks;
    private final boolean mAllowEmptyDocumentPersistence;
    private final boolean mAllowGeolocationOnInsecureOrigins;
    private final boolean mDoNotUpdateSelectionOnMutatingSelectionRange;

    private final boolean mPasswordEchoEnabled;

    // Not accessed by the native side.
    private boolean mBlockNetworkLoads;  // Default depends on permission of embedding APK.
    private boolean mAllowContentUrlAccess = true;
    private boolean mAllowFileUrlAccess = true;
    private int mCacheMode = WebSettings.LOAD_DEFAULT;
    private boolean mShouldFocusFirstNode = true;
    private boolean mGeolocationEnabled = true;
    private boolean mAutoCompleteEnabled = Build.VERSION.SDK_INT < Build.VERSION_CODES.O;
    private boolean mFullscreenSupported;
    private boolean mSupportZoom = true;
    private boolean mBuiltInZoomControls;
    private boolean mDisplayZoomControls = true;

    static class LazyDefaultUserAgent{
        // Lazy Holder pattern
        private static final String sInstance = BisonSettingsJni.get().getDefaultUserAgent();
    }

    // Protects access to settings global fields.
    private static final Object sGlobalContentSettingsLock = new Object();
    // For compatibility with the legacy WebView, we can only enable AppCache when the path is
    // provided. However, we don't use the path, so we just check if we have received it from the
    // client.
    private static boolean sAppCachePathIsSet;

    // The native side of this object. It's lifetime is bounded by the WebContent it is attached to.
    private long mNativeBisonSettings;

    // Custom handler that queues messages to call native code on the UI thread.
    private final EventHandler mEventHandler;

    private static final int MINIMUM_FONT_SIZE = 1;
    private static final int MAXIMUM_FONT_SIZE = 72;

    // Class to handle messages to be processed on the UI thread.
    private class EventHandler {
        // Message id for running a Runnable with mBisonSettingsLock held.
        private static final int RUN_RUNNABLE_BLOCKING = 0;
        // Actual UI thread handler
        private Handler mHandler;
        // Synchronization flag.
        private boolean mSynchronizationPending;

        EventHandler() {
        }

        @SuppressLint("HandlerLeak")
        void bindUiThread() {
            if (mHandler != null) return;
            mHandler = new Handler(ThreadUtils.getUiThreadLooper()) {
                @Override
                public void handleMessage(Message msg) {
                    switch (msg.what) {
                        case RUN_RUNNABLE_BLOCKING:
                            synchronized (mBisonSettingsLock) {
                                if (mNativeBisonSettings != 0) {
                                    ((Runnable) msg.obj).run();
                                }
                                mSynchronizationPending = false;
                                mBisonSettingsLock.notifyAll();
                            }
                            break;
                    }
                }
            };
        }

        void runOnUiThreadBlockingAndLocked(Runnable r) {
            assert Thread.holdsLock(mBisonSettingsLock);
            if (mHandler == null) return;
            if (ThreadUtils.runningOnUiThread()) {
                r.run();
            } else {
                assert !mSynchronizationPending;
                mSynchronizationPending = true;
                mHandler.sendMessage(Message.obtain(null, RUN_RUNNABLE_BLOCKING, r));
                try {
                    while (mSynchronizationPending) {
                        mBisonSettingsLock.wait();
                    }
                } catch (InterruptedException e) {
                    Log.e(TAG, "Interrupted waiting a Runnable to complete", e);
                    mSynchronizationPending = false;
                }
            }
        }

        void maybePostOnUiThread(Runnable r) {
            if (mHandler != null) {
                mHandler.post(r);
            }
        }

        void updateWebkitPreferencesLocked() {
            runOnUiThreadBlockingAndLocked(() -> updateWebkitPreferencesOnUiThreadLocked());
        }

        void updateCookiePolicyLocked() {
            runOnUiThreadBlockingAndLocked(() -> updateCookiePolicyOnUiThreadLocked());
        }

        void updateAllowFileAccessLocked() {
            runOnUiThreadBlockingAndLocked(() -> updateAllowFileAccessOnUiThreadLocked());
        }
    }

    interface ZoomSupportChangeListener {
        public void onGestureZoomSupportChanged(
                boolean supportsDoubleTapZoom, boolean supportsMultiTouchZoom);
    }

    public BisonSettings(Context context, boolean isAccessFromFileURLsGrantedByDefault,
                         boolean supportsLegacyQuirks, boolean allowEmptyDocumentPersistence,
                         boolean allowGeolocationOnInsecureOrigins,
                         boolean doNotUpdateSelectionOnMutatingSelectionRange) {
        boolean hasInternetPermission = context.checkPermission(
                android.Manifest.permission.INTERNET,
                Process.myPid(),
                Process.myUid()) == PackageManager.PERMISSION_GRANTED;
        synchronized (mBisonSettingsLock) {
            mHasInternetPermission = hasInternetPermission;
            mBlockNetworkLoads = !hasInternetPermission;
            mEventHandler = new EventHandler();
            if (isAccessFromFileURLsGrantedByDefault) {
                mAllowUniversalAccessFromFileURLs = true;
                mAllowFileAccessFromFileURLs = true;
            }

            mUserAgent = LazyDefaultUserAgent.sInstance;

            // Best-guess a sensible initial value based on the features supported on the device.
            mSpatialNavigationEnabled = !context.getPackageManager().hasSystemFeature(
                    PackageManager.FEATURE_TOUCHSCREEN);

            // Respect the system setting for password echoing.
            mPasswordEchoEnabled = Settings.System.getInt(context.getContentResolver(),
                    Settings.System.TEXT_SHOW_PASSWORD, 1) == 1;

            // By default, scale the text size by the system font scale factor. Embedders
            // may override this by invoking setTextZoom().
            mTextSizePercent =
                    (int) (mTextSizePercent * context.getResources().getConfiguration().fontScale);

            mSupportLegacyQuirks = supportsLegacyQuirks;
            mAllowEmptyDocumentPersistence = allowEmptyDocumentPersistence;
            mAllowGeolocationOnInsecureOrigins = allowGeolocationOnInsecureOrigins;
            mDoNotUpdateSelectionOnMutatingSelectionRange =
                    doNotUpdateSelectionOnMutatingSelectionRange;
        }
        // Defer initializing the native side until a native WebContents instance is set.
    }

    @CalledByNative
    private void nativeBisonSettingsGone(long nativeBisonSettings) {
        assert mNativeBisonSettings != 0 && mNativeBisonSettings == nativeBisonSettings;
        mNativeBisonSettings = 0;
    }

    @CalledByNative
    private double getDIPScaleLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mDIPScale;
    }

    void setDIPScale(double dipScale) {
        synchronized (mBisonSettingsLock) {
            mDIPScale = dipScale;
            // TODO(joth): This should also be synced over to native side, but right now
            // the setDIPScale call is always followed by a setWebContents() which covers this.
        }
    }

    void setZoomListener(ZoomSupportChangeListener zoomChangeListener) {
        synchronized (mBisonSettingsLock) {
            mZoomChangeListener = zoomChangeListener;
        }
    }

    void setWebContents(WebContents webContents) {
        synchronized (mBisonSettingsLock) {
            if (mNativeBisonSettings != 0) {
                BisonSettingsJni.get().destroy(mNativeBisonSettings, BisonSettings.this);
                assert mNativeBisonSettings == 0;  // nativeBisonSettingsGone should have been called.
            }
            if (webContents != null) {
                mEventHandler.bindUiThread();
                mNativeBisonSettings = BisonSettingsJni.get().init(BisonSettings.this, webContents);
                updateEverythingLocked();
            }
        }
    }

    private void updateEverythingLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        assert mNativeBisonSettings != 0;
        BisonSettingsJni.get().updateEverythingLocked(mNativeBisonSettings, BisonSettings.this);
        onGestureZoomSupportChanged(
                supportsDoubleTapZoomLocked(), supportsMultiTouchZoomLocked());
    }

    /**
     * See {@link android.webkit.WebSettings#setBlockNetworkLoads}.
     */
    public void setBlockNetworkLoads(boolean flag) {
        if (TRACE) Log.i(LOGTAG, "setBlockNetworkLoads=" + flag);
        synchronized (mBisonSettingsLock) {
            if (!flag && !mHasInternetPermission) {
                throw new SecurityException("Permission denied - "
                        + "application missing INTERNET permission");
            }
            mBlockNetworkLoads = flag;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getBlockNetworkLoads}.
     */
    public boolean getBlockNetworkLoads() {
        synchronized (mBisonSettingsLock) {
            return mBlockNetworkLoads;
        }
    }

    /**
     * Enable/disable third party cookies for an BisonContents
     * @param accept true if we should accept third party cookies
     */
    public void setAcceptThirdPartyCookies(boolean accept) {
        if (TRACE) Log.i(LOGTAG, "setAcceptThirdPartyCookies=" + accept);
        synchronized (mBisonSettingsLock) {
            mAcceptThirdPartyCookies = accept;
            mEventHandler.updateCookiePolicyLocked();
        }
    }

    /**
     * Enable/Disable SafeBrowsing per WebView
     * @param enabled true if this WebView should have SafeBrowsing
     */
    public void setSafeBrowsingEnabled(boolean enabled) {
        synchronized (mBisonSettingsLock) {
            mSafeBrowsingEnabled = enabled;
        }
    }

    /**
     * Return whether third party cookies are enabled for an BisonContents
     * @return true if accept third party cookies
     */
    public boolean getAcceptThirdPartyCookies() {
        synchronized (mBisonSettingsLock) {
            return mAcceptThirdPartyCookies;
        }
    }

    @CalledByNative
    private boolean getAcceptThirdPartyCookiesLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mAcceptThirdPartyCookies;
    }

    /**
     * Return whether Safe Browsing has been enabled for the current WebView
     * @return true if SafeBrowsing is enabled
     */
    public boolean getSafeBrowsingEnabled() {
        synchronized (mBisonSettingsLock) {
            Boolean userOptIn = BisonSafeBrowsingConfigHelper.getSafeBrowsingUserOptIn();

            // If we don't know yet what the user's preference is, we go through Safe Browsing logic
            // anyway and correct the assumption before sending data to GMS.
            if (userOptIn != null && !userOptIn) return false;

            if (mSafeBrowsingEnabled == null) {
                return BisonSafeBrowsingConfigHelper.getSafeBrowsingEnabledByManifest();
            }
            return mSafeBrowsingEnabled;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setAllowFileAccess}.
     */
    public void setAllowFileAccess(boolean allow) {
        if (TRACE) Log.i(LOGTAG, "setAllowFileAccess=" + allow);
        synchronized (mBisonSettingsLock) {
            mAllowFileUrlAccess = allow;
            mEventHandler.updateAllowFileAccessLocked();
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getAllowFileAccess}.
     */
    @CalledByNative
    public boolean getAllowFileAccess() {
        synchronized (mBisonSettingsLock) {
            return mAllowFileUrlAccess;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setAllowContentAccess}.
     */
    public void setAllowContentAccess(boolean allow) {
        if (TRACE) Log.i(LOGTAG, "setAllowContentAccess=" + allow);
        synchronized (mBisonSettingsLock) {
            mAllowContentUrlAccess = allow;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getAllowContentAccess}.
     */
    public boolean getAllowContentAccess() {
        synchronized (mBisonSettingsLock) {
            return mAllowContentUrlAccess;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setCacheMode}.
     */
    public void setCacheMode(int mode) {
        if (TRACE) Log.i(LOGTAG, "setCacheMode=" + mode);
        synchronized (mBisonSettingsLock) {
            mCacheMode = mode;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getCacheMode}.
     */
    public int getCacheMode() {
        synchronized (mBisonSettingsLock) {
            return mCacheMode;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setNeedInitialFocus}.
     */
    public void setShouldFocusFirstNode(boolean flag) {
        if (TRACE) Log.i(LOGTAG, "setNeedInitialFocusNode=" + flag);
        synchronized (mBisonSettingsLock) {
            mShouldFocusFirstNode = flag;
        }
    }

    /**
     * See {@link android.webkit.WebView#setInitialScale}.
     */
    public void setInitialPageScale(final float scaleInPercent) {
        if (TRACE) Log.i(LOGTAG, "setInitialScale=" + scaleInPercent);
        synchronized (mBisonSettingsLock) {
            if (mInitialPageScalePercent != scaleInPercent) {
                mInitialPageScalePercent = scaleInPercent;
                mEventHandler.runOnUiThreadBlockingAndLocked(() -> {
                    if (mNativeBisonSettings != 0) {
                        BisonSettingsJni.get().updateInitialPageScaleLocked(
                                mNativeBisonSettings, BisonSettings.this);
                    }
                });
            }
        }
    }

    @CalledByNative
    private float getInitialPageScalePercentLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mInitialPageScalePercent;
    }

    void setSpatialNavigationEnabled(boolean enable) {
        synchronized (mBisonSettingsLock) {
            if (mSpatialNavigationEnabled != enable) {
                mSpatialNavigationEnabled = enable;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    @CalledByNative
    private boolean getSpatialNavigationLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mSpatialNavigationEnabled;
    }

    void setEnableSupportedHardwareAcceleratedFeatures(boolean enable) {
        synchronized (mBisonSettingsLock) {
            if (mEnableSupportedHardwareAcceleratedFeatures != enable) {
                mEnableSupportedHardwareAcceleratedFeatures = enable;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    @CalledByNative
    private boolean getEnableSupportedHardwareAcceleratedFeaturesLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mEnableSupportedHardwareAcceleratedFeatures;
    }

    public void setFullscreenSupported(boolean supported) {
        synchronized (mBisonSettingsLock) {
            if (mFullscreenSupported != supported) {
                mFullscreenSupported = supported;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    @CalledByNative
    private boolean getFullscreenSupportedLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mFullscreenSupported;
    }

    /**
     * See {@link android.webkit.WebSettings#setNeedInitialFocus}.
     */
    public boolean shouldFocusFirstNode() {
        synchronized (mBisonSettingsLock) {
            return mShouldFocusFirstNode;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setGeolocationEnabled}.
     */
    public void setGeolocationEnabled(boolean flag) {
        if (TRACE) Log.i(LOGTAG, "setGeolocationEnabled=" + flag);
        synchronized (mBisonSettingsLock) {
            mGeolocationEnabled = flag;
        }
    }

    /**
     * @return Returns if geolocation is currently enabled.
     */
    boolean getGeolocationEnabled() {
        synchronized (mBisonSettingsLock) {
            return mGeolocationEnabled;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setSaveFormData}.
     */
    public void setSaveFormData(final boolean enable) {
        if (TRACE) Log.i(LOGTAG, "setSaveFormData=" + enable);
        synchronized (mBisonSettingsLock) {
            if (mAutoCompleteEnabled != enable) {
                mAutoCompleteEnabled = enable;
                mEventHandler.runOnUiThreadBlockingAndLocked(() -> {
                    if (mNativeBisonSettings != 0) {
                        BisonSettingsJni.get().updateFormDataPreferencesLocked(
                                mNativeBisonSettings, BisonSettings.this);
                    }
                });
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getSaveFormData}.
     */
    public boolean getSaveFormData() {
        synchronized (mBisonSettingsLock) {
            return getSaveFormDataLocked();
        }
    }

    @CalledByNative
    private boolean getSaveFormDataLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mAutoCompleteEnabled;
    }

    public void setUserAgent(int ua) {
        // Minimal implementation for backwards compatibility: just supports resetting to default.
        if (ua == 0) {
            setUserAgentString(null);
        } else {
            Log.w(LOGTAG, "setUserAgent not supported, ua=" + ua);
        }
    }

    /**
     * @returns the default User-Agent used by each WebContents instance, i.e. unless
     * overridden by {@link #setUserAgentString()}
     */
    public static String getDefaultUserAgent() {
        return LazyDefaultUserAgent.sInstance;
    }

    @CalledByNative
    private static boolean getAllowSniffingFileUrls() {
        // Don't allow sniffing file:// URLs for MIME type if the application targets P or later.
        return ContextUtils.getApplicationContext().getApplicationInfo().targetSdkVersion
                < Build.VERSION_CODES.P;
    }

    /**
     * See {@link android.webkit.WebSettings#setUserAgentString}.
     */
    public void setUserAgentString(String ua) {
        if (TRACE) Log.i(LOGTAG, "setUserAgentString=" + ua);
        synchronized (mBisonSettingsLock) {
            final String oldUserAgent = mUserAgent;
            if (ua == null || ua.length() == 0) {
                mUserAgent = LazyDefaultUserAgent.sInstance;
            } else {
                mUserAgent = ua;
            }
            if (!oldUserAgent.equals(mUserAgent)) {
                mEventHandler.runOnUiThreadBlockingAndLocked(() -> {
                    if (mNativeBisonSettings != 0) {
                        BisonSettingsJni.get().updateUserAgentLocked(
                                mNativeBisonSettings, BisonSettings.this);
                    }
                });
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getUserAgentString}.
     */
    public String getUserAgentString() {
        synchronized (mBisonSettingsLock) {
            return getUserAgentLocked();
        }
    }

    @CalledByNative
    private String getUserAgentLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mUserAgent;
    }

    /**
     * See {@link android.webkit.WebSettings#setLoadWithOverviewMode}.
     */
    public void setLoadWithOverviewMode(boolean overview) {
        if (TRACE) Log.i(LOGTAG, "setLoadWithOverviewMode=" + overview);
        synchronized (mBisonSettingsLock) {
            if (mLoadWithOverviewMode != overview) {
                mLoadWithOverviewMode = overview;
                mEventHandler.runOnUiThreadBlockingAndLocked(() -> {
                    if (mNativeBisonSettings != 0) {
                        updateWebkitPreferencesOnUiThreadLocked();
                        BisonSettingsJni.get().resetScrollAndScaleState(
                                mNativeBisonSettings, BisonSettings.this);
                    }
                });
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getLoadWithOverviewMode}.
     */
    public boolean getLoadWithOverviewMode() {
        synchronized (mBisonSettingsLock) {
            return getLoadWithOverviewModeLocked();
        }
    }

    @CalledByNative
    private boolean getLoadWithOverviewModeLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mLoadWithOverviewMode;
    }

    /**
     * See {@link android.webkit.WebSettings#setTextZoom}.
     */
    public void setTextZoom(final int textZoom) {
        if (TRACE) Log.i(LOGTAG, "setTextZoom=" + textZoom);
        synchronized (mBisonSettingsLock) {
            if (mTextSizePercent != textZoom) {
                mTextSizePercent = textZoom;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getTextZoom}.
     */
    public int getTextZoom() {
        synchronized (mBisonSettingsLock) {
            return getTextSizePercentLocked();
        }
    }

    @CalledByNative
    private int getTextSizePercentLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mTextSizePercent;
    }

    /**
     * See {@link android.webkit.WebSettings#setStandardFontFamily}.
     */
    public void setStandardFontFamily(String font) {
        if (TRACE) Log.i(LOGTAG, "setStandardFontFamily=" + font);
        synchronized (mBisonSettingsLock) {
            if (font != null && !mStandardFontFamily.equals(font)) {
                mStandardFontFamily = font;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getStandardFontFamily}.
     */
    public String getStandardFontFamily() {
        synchronized (mBisonSettingsLock) {
            return getStandardFontFamilyLocked();
        }
    }

    @CalledByNative
    private String getStandardFontFamilyLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mStandardFontFamily;
    }

    /**
     * See {@link android.webkit.WebSettings#setFixedFontFamily}.
     */
    public void setFixedFontFamily(String font) {
        if (TRACE) Log.i(LOGTAG, "setFixedFontFamily=" + font);
        synchronized (mBisonSettingsLock) {
            if (font != null && !mFixedFontFamily.equals(font)) {
                mFixedFontFamily = font;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getFixedFontFamily}.
     */
    public String getFixedFontFamily() {
        synchronized (mBisonSettingsLock) {
            return getFixedFontFamilyLocked();
        }
    }

    @CalledByNative
    private String getFixedFontFamilyLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mFixedFontFamily;
    }

    /**
     * See {@link android.webkit.WebSettings#setSansSerifFontFamily}.
     */
    public void setSansSerifFontFamily(String font) {
        if (TRACE) Log.i(LOGTAG, "setSansSerifFontFamily=" + font);
        synchronized (mBisonSettingsLock) {
            if (font != null && !mSansSerifFontFamily.equals(font)) {
                mSansSerifFontFamily = font;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getSansSerifFontFamily}.
     */
    public String getSansSerifFontFamily() {
        synchronized (mBisonSettingsLock) {
            return getSansSerifFontFamilyLocked();
        }
    }

    @CalledByNative
    private String getSansSerifFontFamilyLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mSansSerifFontFamily;
    }

    /**
     * See {@link android.webkit.WebSettings#setSerifFontFamily}.
     */
    public void setSerifFontFamily(String font) {
        if (TRACE) Log.i(LOGTAG, "setSerifFontFamily=" + font);
        synchronized (mBisonSettingsLock) {
            if (font != null && !mSerifFontFamily.equals(font)) {
                mSerifFontFamily = font;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getSerifFontFamily}.
     */
    public String getSerifFontFamily() {
        synchronized (mBisonSettingsLock) {
            return getSerifFontFamilyLocked();
        }
    }

    @CalledByNative
    private String getSerifFontFamilyLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mSerifFontFamily;
    }

    /**
     * See {@link android.webkit.WebSettings#setCursiveFontFamily}.
     */
    public void setCursiveFontFamily(String font) {
        if (TRACE) Log.i(LOGTAG, "setCursiveFontFamily=" + font);
        synchronized (mBisonSettingsLock) {
            if (font != null && !mCursiveFontFamily.equals(font)) {
                mCursiveFontFamily = font;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getCursiveFontFamily}.
     */
    public String getCursiveFontFamily() {
        synchronized (mBisonSettingsLock) {
            return getCursiveFontFamilyLocked();
        }
    }

    @CalledByNative
    private String getCursiveFontFamilyLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mCursiveFontFamily;
    }

    /**
     * See {@link android.webkit.WebSettings#setFantasyFontFamily}.
     */
    public void setFantasyFontFamily(String font) {
        if (TRACE) Log.i(LOGTAG, "setFantasyFontFamily=" + font);
        synchronized (mBisonSettingsLock) {
            if (font != null && !mFantasyFontFamily.equals(font)) {
                mFantasyFontFamily = font;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getFantasyFontFamily}.
     */
    public String getFantasyFontFamily() {
        synchronized (mBisonSettingsLock) {
            return getFantasyFontFamilyLocked();
        }
    }

    @CalledByNative
    private String getFantasyFontFamilyLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mFantasyFontFamily;
    }

    /**
     * See {@link android.webkit.WebSettings#setMinimumFontSize}.
     */
    public void setMinimumFontSize(int size) {
        if (TRACE) Log.i(LOGTAG, "setMinimumFontSize=" + size);
        synchronized (mBisonSettingsLock) {
            size = clipFontSize(size);
            if (mMinimumFontSize != size) {
                mMinimumFontSize = size;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getMinimumFontSize}.
     */
    public int getMinimumFontSize() {
        synchronized (mBisonSettingsLock) {
            return getMinimumFontSizeLocked();
        }
    }

    @CalledByNative
    private int getMinimumFontSizeLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mMinimumFontSize;
    }

    /**
     * See {@link android.webkit.WebSettings#setMinimumLogicalFontSize}.
     */
    public void setMinimumLogicalFontSize(int size) {
        if (TRACE) Log.i(LOGTAG, "setMinimumLogicalFontSize=" + size);
        synchronized (mBisonSettingsLock) {
            size = clipFontSize(size);
            if (mMinimumLogicalFontSize != size) {
                mMinimumLogicalFontSize = size;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getMinimumLogicalFontSize}.
     */
    public int getMinimumLogicalFontSize() {
        synchronized (mBisonSettingsLock) {
            return getMinimumLogicalFontSizeLocked();
        }
    }

    @CalledByNative
    private int getMinimumLogicalFontSizeLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mMinimumLogicalFontSize;
    }

    /**
     * See {@link android.webkit.WebSettings#setDefaultFontSize}.
     */
    public void setDefaultFontSize(int size) {
        if (TRACE) Log.i(LOGTAG, "setDefaultFontSize=" + size);
        synchronized (mBisonSettingsLock) {
            size = clipFontSize(size);
            if (mDefaultFontSize != size) {
                mDefaultFontSize = size;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getDefaultFontSize}.
     */
    public int getDefaultFontSize() {
        synchronized (mBisonSettingsLock) {
            return getDefaultFontSizeLocked();
        }
    }

    @CalledByNative
    private int getDefaultFontSizeLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mDefaultFontSize;
    }

    /**
     * See {@link android.webkit.WebSettings#setDefaultFixedFontSize}.
     */
    public void setDefaultFixedFontSize(int size) {
        if (TRACE) Log.i(LOGTAG, "setDefaultFixedFontSize=" + size);
        synchronized (mBisonSettingsLock) {
            size = clipFontSize(size);
            if (mDefaultFixedFontSize != size) {
                mDefaultFixedFontSize = size;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getDefaultFixedFontSize}.
     */
    public int getDefaultFixedFontSize() {
        synchronized (mBisonSettingsLock) {
            return getDefaultFixedFontSizeLocked();
        }
    }

    @CalledByNative
    private int getDefaultFixedFontSizeLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mDefaultFixedFontSize;
    }

    /**
     * See {@link android.webkit.WebSettings#setJavaScriptEnabled}.
     */
    public void setJavaScriptEnabled(boolean flag) {
        if (TRACE) Log.i(LOGTAG, "setJavaScriptEnabled=" + flag);
        synchronized (mBisonSettingsLock) {
            if (mJavaScriptEnabled != flag) {
                mJavaScriptEnabled = flag;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setAllowUniversalAccessFromFileURLs}.
     */
    public void setAllowUniversalAccessFromFileURLs(boolean flag) {
        if (TRACE) Log.i(LOGTAG, "setAllowUniversalAccessFromFileURLs=" + flag);
        synchronized (mBisonSettingsLock) {
            if (mAllowUniversalAccessFromFileURLs != flag) {
                mAllowUniversalAccessFromFileURLs = flag;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setAllowFileAccessFromFileURLs}.
     */
    public void setAllowFileAccessFromFileURLs(boolean flag) {
        if (TRACE) Log.i(LOGTAG, "setAllowFileAccessFromFileURLs=" + flag);
        synchronized (mBisonSettingsLock) {
            if (mAllowFileAccessFromFileURLs != flag) {
                mAllowFileAccessFromFileURLs = flag;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setLoadsImagesAutomatically}.
     */
    public void setLoadsImagesAutomatically(boolean flag) {
        if (TRACE) Log.i(LOGTAG, "setLoadsImagesAutomatically=" + flag);
        synchronized (mBisonSettingsLock) {
            if (mLoadsImagesAutomatically != flag) {
                mLoadsImagesAutomatically = flag;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getLoadsImagesAutomatically}.
     */
    public boolean getLoadsImagesAutomatically() {
        synchronized (mBisonSettingsLock) {
            return getLoadsImagesAutomaticallyLocked();
        }
    }

    @CalledByNative
    private boolean getLoadsImagesAutomaticallyLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mLoadsImagesAutomatically;
    }

    /**
     * See {@link android.webkit.WebSettings#setImagesEnabled}.
     */
    public void setImagesEnabled(boolean flag) {
        if (TRACE) Log.i(LOGTAG, "setBlockNetworkImage=" + flag);
        synchronized (mBisonSettingsLock) {
            if (mImagesEnabled != flag) {
                mImagesEnabled = flag;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getImagesEnabled}.
     */
    public boolean getImagesEnabled() {
        synchronized (mBisonSettingsLock) {
            return mImagesEnabled;
        }
    }

    @CalledByNative
    private boolean getImagesEnabledLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mImagesEnabled;
    }

    /**
     * See {@link android.webkit.WebSettings#getJavaScriptEnabled}.
     */
    public boolean getJavaScriptEnabled() {
        synchronized (mBisonSettingsLock) {
            return mJavaScriptEnabled;
        }
    }

    @CalledByNative
    private boolean getJavaScriptEnabledLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mJavaScriptEnabled;
    }

    /**
     * See {@link android.webkit.WebSettings#getAllowUniversalAccessFromFileURLs}.
     */
    public boolean getAllowUniversalAccessFromFileURLs() {
        synchronized (mBisonSettingsLock) {
            return getAllowUniversalAccessFromFileURLsLocked();
        }
    }

    @CalledByNative
    private boolean getAllowUniversalAccessFromFileURLsLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mAllowUniversalAccessFromFileURLs;
    }

    /**
     * See {@link android.webkit.WebSettings#getAllowFileAccessFromFileURLs}.
     */
    public boolean getAllowFileAccessFromFileURLs() {
        synchronized (mBisonSettingsLock) {
            return getAllowFileAccessFromFileURLsLocked();
        }
    }

    @CalledByNative
    private boolean getAllowFileAccessFromFileURLsLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mAllowFileAccessFromFileURLs;
    }

    /**
     * See {@link android.webkit.WebSettings#setJavaScriptCanOpenWindowsAutomatically}.
     */
    public void setJavaScriptCanOpenWindowsAutomatically(boolean flag) {
        if (TRACE) Log.i(LOGTAG, "setJavaScriptCanOpenWindowsAutomatically=" + flag);
        synchronized (mBisonSettingsLock) {
            if (mJavaScriptCanOpenWindowsAutomatically != flag) {
                mJavaScriptCanOpenWindowsAutomatically = flag;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getJavaScriptCanOpenWindowsAutomatically}.
     */
    public boolean getJavaScriptCanOpenWindowsAutomatically() {
        synchronized (mBisonSettingsLock) {
            return getJavaScriptCanOpenWindowsAutomaticallyLocked();
        }
    }

    @CalledByNative
    private boolean getJavaScriptCanOpenWindowsAutomaticallyLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mJavaScriptCanOpenWindowsAutomatically;
    }

    /**
     * See {@link android.webkit.WebSettings#setLayoutAlgorithm}.
     */
    public void setLayoutAlgorithm(@LayoutAlgorithm int l) {
        if (TRACE) Log.i(LOGTAG, "setLayoutAlgorithm=" + l);
        synchronized (mBisonSettingsLock) {
            if (mLayoutAlgorithm != l) {
                mLayoutAlgorithm = l;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getLayoutAlgorithm}.
     */
    @LayoutAlgorithm
    public int getLayoutAlgorithm() {
        synchronized (mBisonSettingsLock) {
            return mLayoutAlgorithm;
        }
    }

    /**
     * Gets whether Text Auto-sizing layout algorithm is enabled.
     *
     * @return true if Text Auto-sizing layout algorithm is enabled
     */
    @CalledByNative
    private boolean getTextAutosizingEnabledLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mLayoutAlgorithm == LAYOUT_ALGORITHM_TEXT_AUTOSIZING;
    }

    /**
     * See {@link android.webkit.WebSettings#setSupportMultipleWindows}.
     */
    public void setSupportMultipleWindows(boolean support) {
        if (TRACE) Log.i(LOGTAG, "setSupportMultipleWindows=" + support);
        synchronized (mBisonSettingsLock) {
            if (mSupportMultipleWindows != support) {
                mSupportMultipleWindows = support;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#supportMultipleWindows}.
     */
    public boolean supportMultipleWindows() {
        synchronized (mBisonSettingsLock) {
            return mSupportMultipleWindows;
        }
    }

    @CalledByNative
    private boolean getSupportMultipleWindowsLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mSupportMultipleWindows;
    }

    @CalledByNative
    private boolean getCSSHexAlphaColorEnabledLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mCSSHexAlphaColorEnabled;
    }

    public void setCSSHexAlphaColorEnabled(boolean enabled) {
        synchronized (mBisonSettingsLock) {
            if (mCSSHexAlphaColorEnabled != enabled) {
                mCSSHexAlphaColorEnabled = enabled;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    @CalledByNative
    private boolean getScrollTopLeftInteropEnabledLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mScrollTopLeftInteropEnabled;
    }

    public void setScrollTopLeftInteropEnabled(boolean enabled) {
        synchronized (mBisonSettingsLock) {
            if (mScrollTopLeftInteropEnabled != enabled) {
                mScrollTopLeftInteropEnabled = enabled;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    @CalledByNative
    private boolean getWillSuppressErrorPageLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mWillSuppressErrorPage;
    }

    public boolean getWillSuppressErrorPage() {
        synchronized (mBisonSettingsLock) {
            return getWillSuppressErrorPageLocked();
        }
    }

    public void setWillSuppressErrorPage(boolean suppressed) {
        synchronized (mBisonSettingsLock) {
            if (mWillSuppressErrorPage == suppressed) return;

            mWillSuppressErrorPage = suppressed;
            updateWillSuppressErrorStateLocked();
        }
    }

    private void updateWillSuppressErrorStateLocked() {
        mEventHandler.runOnUiThreadBlockingAndLocked(() -> {
            assert Thread.holdsLock(mBisonSettingsLock);
            assert mNativeBisonSettings != 0;
            BisonSettingsJni.get().updateWillSuppressErrorStateLocked(
                    mNativeBisonSettings, BisonSettings.this);
        });
    }

    @CalledByNative
    private boolean getSupportLegacyQuirksLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mSupportLegacyQuirks;
    }

    @CalledByNative
    private boolean getAllowEmptyDocumentPersistenceLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mAllowEmptyDocumentPersistence;
    }

    @CalledByNative
    private boolean getAllowGeolocationOnInsecureOrigins() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mAllowGeolocationOnInsecureOrigins;
    }

    @CalledByNative
    private boolean getDoNotUpdateSelectionOnMutatingSelectionRange() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mDoNotUpdateSelectionOnMutatingSelectionRange;
    }

    /**
     * See {@link android.webkit.WebSettings#setUseWideViewPort}.
     */
    public void setUseWideViewPort(boolean use) {
        if (TRACE) Log.i(LOGTAG, "setUseWideViewPort=" + use);
        synchronized (mBisonSettingsLock) {
            if (mUseWideViewport != use) {
                mUseWideViewport = use;
                onGestureZoomSupportChanged(
                        supportsDoubleTapZoomLocked(), supportsMultiTouchZoomLocked());
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getUseWideViewPort}.
     */
    public boolean getUseWideViewPort() {
        synchronized (mBisonSettingsLock) {
            return getUseWideViewportLocked();
        }
    }

    @CalledByNative
    private boolean getUseWideViewportLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mUseWideViewport;
    }

    public void setZeroLayoutHeightDisablesViewportQuirk(boolean enabled) {
        synchronized (mBisonSettingsLock) {
            if (mZeroLayoutHeightDisablesViewportQuirk != enabled) {
                mZeroLayoutHeightDisablesViewportQuirk = enabled;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    public boolean getZeroLayoutHeightDisablesViewportQuirk() {
        synchronized (mBisonSettingsLock) {
            return getZeroLayoutHeightDisablesViewportQuirkLocked();
        }
    }

    @CalledByNative
    private boolean getZeroLayoutHeightDisablesViewportQuirkLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mZeroLayoutHeightDisablesViewportQuirk;
    }

    public void setForceZeroLayoutHeight(boolean enabled) {
        synchronized (mBisonSettingsLock) {
            if (mForceZeroLayoutHeight != enabled) {
                mForceZeroLayoutHeight = enabled;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    public boolean getForceZeroLayoutHeight() {
        synchronized (mBisonSettingsLock) {
            return getForceZeroLayoutHeightLocked();
        }
    }

    @CalledByNative
    private boolean getForceZeroLayoutHeightLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mForceZeroLayoutHeight;
    }

    @CalledByNative
    private boolean getPasswordEchoEnabledLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mPasswordEchoEnabled;
    }

    /**
     * See {@link android.webkit.WebSettings#setAppCacheEnabled}.
     */
    public void setAppCacheEnabled(boolean flag) {
        if (TRACE) Log.i(LOGTAG, "setAppCacheEnabled=" + flag);
        synchronized (mBisonSettingsLock) {
            if (mAppCacheEnabled != flag) {
                mAppCacheEnabled = flag;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setAppCachePath}.
     */
    public void setAppCachePath(String path) {
        if (TRACE) Log.i(LOGTAG, "setAppCachePath=" + path);
        boolean needToSync = false;
        synchronized (sGlobalContentSettingsLock) {
            // AppCachePath can only be set once.
            if (!sAppCachePathIsSet && path != null && !path.isEmpty()) {
                sAppCachePathIsSet = true;
                needToSync = true;
            }
        }
        // The obvious problem here is that other WebViews will not be updated,
        // until they execute synchronization from Java to the native side.
        // But this is the same behaviour as it was in the legacy WebView.
        if (needToSync) {
            synchronized (mBisonSettingsLock) {
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * Gets whether Application Cache is enabled.
     *
     * @return true if Application Cache is enabled
     */
    @CalledByNative
    private boolean getAppCacheEnabledLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        if (!mAppCacheEnabled) {
            return false;
        }
        synchronized (sGlobalContentSettingsLock) {
            return sAppCachePathIsSet;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setDomStorageEnabled}.
     */
    public void setDomStorageEnabled(boolean flag) {
        if (TRACE) Log.i(LOGTAG, "setDomStorageEnabled=" + flag);
        synchronized (mBisonSettingsLock) {
            if (mDomStorageEnabled != flag) {
                mDomStorageEnabled = flag;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getDomStorageEnabled}.
     */
    public boolean getDomStorageEnabled() {
        synchronized (mBisonSettingsLock) {
            return mDomStorageEnabled;
        }
    }

    @CalledByNative
    private boolean getDomStorageEnabledLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mDomStorageEnabled;
    }

    /**
     * See {@link android.webkit.WebSettings#setDatabaseEnabled}.
     */
    public void setDatabaseEnabled(boolean flag) {
        if (TRACE) Log.i(LOGTAG, "setDatabaseEnabled=" + flag);
        synchronized (mBisonSettingsLock) {
            if (mDatabaseEnabled != flag) {
                mDatabaseEnabled = flag;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getDatabaseEnabled}.
     */
    public boolean getDatabaseEnabled() {
        synchronized (mBisonSettingsLock) {
            return mDatabaseEnabled;
        }
    }

    @CalledByNative
    private boolean getDatabaseEnabledLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mDatabaseEnabled;
    }

    /**
     * See {@link android.webkit.WebSettings#setDefaultTextEncodingName}.
     */
    public void setDefaultTextEncodingName(String encoding) {
        if (TRACE) Log.i(LOGTAG, "setDefaultTextEncodingName=" + encoding);
        synchronized (mBisonSettingsLock) {
            if (encoding != null && !mDefaultTextEncoding.equals(encoding)) {
                mDefaultTextEncoding = encoding;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getDefaultTextEncodingName}.
     */
    public String getDefaultTextEncodingName() {
        synchronized (mBisonSettingsLock) {
            return getDefaultTextEncodingLocked();
        }
    }

    @CalledByNative
    private String getDefaultTextEncodingLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mDefaultTextEncoding;
    }

    /**
     * See {@link android.webkit.WebSettings#setMediaPlaybackRequiresUserGesture}.
     */
    public void setMediaPlaybackRequiresUserGesture(boolean require) {
        if (TRACE) Log.i(LOGTAG, "setMediaPlaybackRequiresUserGesture=" + require);
        synchronized (mBisonSettingsLock) {
            if (mMediaPlaybackRequiresUserGesture != require) {
                mMediaPlaybackRequiresUserGesture = require;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getMediaPlaybackRequiresUserGesture}.
     */
    public boolean getMediaPlaybackRequiresUserGesture() {
        synchronized (mBisonSettingsLock) {
            return getMediaPlaybackRequiresUserGestureLocked();
        }
    }

    @CalledByNative
    private boolean getMediaPlaybackRequiresUserGestureLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mMediaPlaybackRequiresUserGesture;
    }

    /**
     * See {@link android.webkit.WebSettings#setDefaultVideoPosterURL}.
     */
    public void setDefaultVideoPosterURL(String url) {
        synchronized (mBisonSettingsLock) {
            if ((mDefaultVideoPosterURL != null && !mDefaultVideoPosterURL.equals(url))
                    || (mDefaultVideoPosterURL == null && url != null)) {
                mDefaultVideoPosterURL = url;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getDefaultVideoPosterURL}.
     */
    public String getDefaultVideoPosterURL() {
        synchronized (mBisonSettingsLock) {
            return getDefaultVideoPosterURLLocked();
        }
    }

    @CalledByNative
    private String getDefaultVideoPosterURLLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mDefaultVideoPosterURL;
    }

    private void onGestureZoomSupportChanged(
            final boolean supportsDoubleTapZoom, final boolean supportsMultiTouchZoom) {
        // Always post asynchronously here, to avoid doubling back onto the caller.
        mEventHandler.maybePostOnUiThread(() -> {
            synchronized (mBisonSettingsLock) {
                if (mZoomChangeListener != null) {
                    mZoomChangeListener.onGestureZoomSupportChanged(
                            supportsDoubleTapZoom, supportsMultiTouchZoom);
                }
            }
        });
    }

    /**
     * See {@link android.webkit.WebSettings#setSupportZoom}.
     */
    public void setSupportZoom(boolean support) {
        if (TRACE) Log.i(LOGTAG, "setSupportZoom=" + support);
        synchronized (mBisonSettingsLock) {
            if (mSupportZoom != support) {
                mSupportZoom = support;
                onGestureZoomSupportChanged(
                        supportsDoubleTapZoomLocked(), supportsMultiTouchZoomLocked());
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#supportZoom}.
     */
    public boolean supportZoom() {
        synchronized (mBisonSettingsLock) {
            return mSupportZoom;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setBuiltInZoomControls}.
     */
    public void setBuiltInZoomControls(boolean enabled) {
        if (TRACE) Log.i(LOGTAG, "setBuiltInZoomControls=" + enabled);
        synchronized (mBisonSettingsLock) {
            if (mBuiltInZoomControls != enabled) {
                mBuiltInZoomControls = enabled;
                onGestureZoomSupportChanged(
                        supportsDoubleTapZoomLocked(), supportsMultiTouchZoomLocked());
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getBuiltInZoomControls}.
     */
    public boolean getBuiltInZoomControls() {
        synchronized (mBisonSettingsLock) {
            return mBuiltInZoomControls;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setDisplayZoomControls}.
     */
    public void setDisplayZoomControls(boolean enabled) {
        if (TRACE) Log.i(LOGTAG, "setDisplayZoomControls=" + enabled);
        synchronized (mBisonSettingsLock) {
            mDisplayZoomControls = enabled;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getDisplayZoomControls}.
     */
    public boolean getDisplayZoomControls() {
        synchronized (mBisonSettingsLock) {
            return mDisplayZoomControls;
        }
    }

    public void setMixedContentMode(int mode) {
        synchronized (mBisonSettingsLock) {
            if (mMixedContentMode != mode) {
                mMixedContentMode = mode;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    public int getMixedContentMode() {
        synchronized (mBisonSettingsLock) {
            return mMixedContentMode;
        }
    }

    @ForceDarkMode
    public int getForceDarkMode() {
        synchronized (mBisonSettingsLock) {
            return getForceDarkModeLocked();
        }
    }

    @CalledByNative
    @ForceDarkMode
    public int getForceDarkModeLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mForceDarkMode;
    }

    public void setForceDarkMode(@ForceDarkMode int forceDarkMode) {
        synchronized (mBisonSettingsLock) {
            if (mForceDarkMode != forceDarkMode) {
                mForceDarkMode = forceDarkMode;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    @ForceDarkBehavior
    public int getForceDarkBehavior() {
        synchronized (mBisonSettingsLock) {
            return getForceDarkBehaviorLocked();
        }
    }

    @CalledByNative
    @ForceDarkBehavior
    public int getForceDarkBehaviorLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mForceDarkBehavior;
    }

    public void setForceDarkBehavior(@ForceDarkBehavior int forceDarkBehavior) {
        synchronized (mBisonSettingsLock) {
            if (mForceDarkBehavior != forceDarkBehavior) {
                mForceDarkBehavior = forceDarkBehavior;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    @CalledByNative
    private boolean getAllowRunningInsecureContentLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mMixedContentMode == WebSettings.MIXED_CONTENT_ALWAYS_ALLOW;
    }

    @CalledByNative
    private boolean getUseStricMixedContentCheckingLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mMixedContentMode == WebSettings.MIXED_CONTENT_NEVER_ALLOW;
    }

    public boolean getOffscreenPreRaster() {
        synchronized (mBisonSettingsLock) {
            return getOffscreenPreRasterLocked();
        }
    }

    @CalledByNative
    private boolean getOffscreenPreRasterLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mOffscreenPreRaster;
    }

    /**
     * Sets whether this WebView should raster tiles when it is
     * offscreen but attached to window. Turning this on can avoid
     * rendering artifacts when animating an offscreen WebView on-screen.
     * In particular, insertVisualStateCallback requires this mode to function.
     * Offscreen WebViews in this mode uses more memory. Please follow
     * these guidelines to limit memory usage:
     * - Webview size should be not be larger than the device screen size.
     * - Limit simple mode to a small number of webviews. Use it for
     *   visible webviews and webviews about to be animated to visible.
     */
    public void setOffscreenPreRaster(boolean enabled) {
        synchronized (mBisonSettingsLock) {
            if (enabled != mOffscreenPreRaster) {
                mOffscreenPreRaster = enabled;
                mEventHandler.runOnUiThreadBlockingAndLocked(() -> {
                    if (mNativeBisonSettings != 0) {
                        BisonSettingsJni.get().updateOffscreenPreRasterLocked(
                                mNativeBisonSettings, BisonSettings.this);
                    }
                });
            }
        }
    }

    public int getDisabledActionModeMenuItems() {
        synchronized (mBisonSettingsLock) {
            return mDisabledMenuItems;
        }
    }

    public void setDisabledActionModeMenuItems(int menuItems) {
        synchronized (mBisonSettingsLock) {
            mDisabledMenuItems = menuItems;
        }
    }

    public void updateAcceptLanguages() {
        synchronized (mBisonSettingsLock) {
            mEventHandler.runOnUiThreadBlockingAndLocked(() -> {
                if (mNativeBisonSettings != 0) {
                    BisonSettingsJni.get().updateRendererPreferencesLocked(
                            mNativeBisonSettings, BisonSettings.this);
                }
            });
        }
    }

    @CalledByNative
    private boolean supportsDoubleTapZoomLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mSupportZoom && mBuiltInZoomControls && mUseWideViewport;
    }

    private boolean supportsMultiTouchZoomLocked() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return mSupportZoom && mBuiltInZoomControls;
    }

    boolean supportsMultiTouchZoom() {
        synchronized (mBisonSettingsLock) {
            return supportsMultiTouchZoomLocked();
        }
    }

    boolean shouldDisplayZoomControls() {
        synchronized (mBisonSettingsLock) {
            return supportsMultiTouchZoomLocked() && mDisplayZoomControls;
        }
    }

    private int clipFontSize(int size) {
        if (size < MINIMUM_FONT_SIZE) {
            return MINIMUM_FONT_SIZE;
        } else if (size > MAXIMUM_FONT_SIZE) {
            return MAXIMUM_FONT_SIZE;
        }
        return size;
    }

    @CalledByNative
    private boolean getRecordFullDocument() {
        assert Thread.holdsLock(mBisonSettingsLock);
        return BisonContentsStatics.getRecordFullDocument();
    }

    @CalledByNative
    private void updateEverything() {
        synchronized (mBisonSettingsLock) {
            updateEverythingLocked();
        }
    }

    @CalledByNative
    private void populateWebPreferences(long webPrefsPtr) {
        synchronized (mBisonSettingsLock) {
            assert mNativeBisonSettings != 0;
            BisonSettingsJni.get().populateWebPreferencesLocked(
                    mNativeBisonSettings, BisonSettings.this, webPrefsPtr);
        }
    }

    private void updateWebkitPreferencesOnUiThreadLocked() {
        assert mEventHandler.mHandler != null;
        ThreadUtils.assertOnUiThread();
        if (mNativeBisonSettings != 0) {
            BisonSettingsJni.get().updateWebkitPreferencesLocked(mNativeBisonSettings, BisonSettings.this);
        }
    }

    private void updateCookiePolicyOnUiThreadLocked() {
        assert mEventHandler.mHandler != null;
        ThreadUtils.assertOnUiThread();
        if (mNativeBisonSettings != 0) {
            BisonSettingsJni.get().updateCookiePolicyLocked(mNativeBisonSettings, BisonSettings.this);
        }
    }

    private void updateAllowFileAccessOnUiThreadLocked() {
        assert mEventHandler.mHandler != null;
        ThreadUtils.assertOnUiThread();
        if (mNativeBisonSettings != 0) {
            BisonSettingsJni.get().updateAllowFileAccessLocked(mNativeBisonSettings, BisonSettings.this);
        }
    }

    @NativeMethods
    interface Natives {
        long init(BisonSettings caller, WebContents webContents);
        void destroy(long nativeBisonSettings, BisonSettings caller);
        void populateWebPreferencesLocked(
                long nativeBisonSettings, BisonSettings caller, long webPrefsPtr);
        void resetScrollAndScaleState(long nativeBisonSettings, BisonSettings caller);
        void updateEverythingLocked(long nativeBisonSettings, BisonSettings caller);
        void updateInitialPageScaleLocked(long nativeBisonSettings, BisonSettings caller);
        void updateUserAgentLocked(long nativeBisonSettings, BisonSettings caller);
        void updateWebkitPreferencesLocked(long nativeBisonSettings, BisonSettings caller);
        String getDefaultUserAgent();
        void updateFormDataPreferencesLocked(long nativeBisonSettings, BisonSettings caller);
        void updateRendererPreferencesLocked(long nativeBisonSettings, BisonSettings caller);
        void updateOffscreenPreRasterLocked(long nativeBisonSettings, BisonSettings caller);
        void updateWillSuppressErrorStateLocked(long nativeBisonSettings, BisonSettings caller);
        void updateCookiePolicyLocked(long nativeBisonSettings, BisonSettings caller);
        void updateAllowFileAccessLocked(long nativeBisonSettings, BisonSettings caller);
    }
}
