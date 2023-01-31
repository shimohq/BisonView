package im.shimo.bison.internal;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.os.Process;
import android.provider.Settings;
import android.webkit.WebSettings;

import androidx.annotation.IntDef;
import androidx.annotation.RestrictTo;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.content_public.browser.WebContents;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
@JNINamespace("bison")
public class BvSettings {

    private static final String LOGTAG = BvSettings.class.getSimpleName();
    private static final boolean TRACE = false;

    /* See {@link android.webkit.WebSettings}. */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ LAYOUT_ALGORITHM_NORMAL,
            /* See {@link android.webkit.WebSettings}. */
            LAYOUT_ALGORITHM_SINGLE_COLUMN,
            /* See {@link android.webkit.WebSettings}. */
            LAYOUT_ALGORITHM_NARROW_COLUMNS, LAYOUT_ALGORITHM_TEXT_AUTOSIZING })
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
    public static final int FORCE_DARK_MODES_COUNT = 3;

    @ForceDarkMode
    private int mForceDarkMode = ForceDarkMode.FORCE_DARK_AUTO;

    private boolean mAlgorithmicDarkeningAllowed;

    public static final int FORCE_DARK_ONLY = ForceDarkBehavior.FORCE_DARK_ONLY;
    public static final int MEDIA_QUERY_ONLY = ForceDarkBehavior.MEDIA_QUERY_ONLY;
    // This option requires RuntimeEnabledFeatures::MetaColorSchemeEnabled()
    public static final int PREFER_MEDIA_QUERY_OVER_FORCE_DARK = ForceDarkBehavior.PREFER_MEDIA_QUERY_OVER_FORCE_DARK;

    @ForceDarkBehavior
    private int mForceDarkBehavior = ForceDarkBehavior.PREFER_MEDIA_QUERY_OVER_FORCE_DARK;

    private Set<String> mRequestedWithHeaderAllowedOriginRules;

    private Context mContext;

    // This class must be created on the UI thread. Afterwards, it can be
    // used from any thread. Internally, the class uses a message queue
    // to call native code on the UI thread only.

    // Values passed in on construction.
    private final boolean mHasInternetPermission;

    private ZoomSupportChangeListener mZoomChangeListener;
    private double mDIPScale = 1.0;

    // Lock to protect all settings.
    private final Object mSettingsLock = new Object();

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
    private boolean mDomStorageEnabled;
    private boolean mDatabaseEnabled;
    private boolean mUseWideViewport;
    private boolean mZeroLayoutHeightDisablesViewportQuirk;
    private boolean mForceZeroLayoutHeight;
    private boolean mLoadWithOverviewMode;
    private boolean mMediaPlaybackRequiresUserGesture = true;
    private String mDefaultVideoPosterURL;
    private float mInitialPageScalePercent;
    private boolean mSpatialNavigationEnabled; // Default depends on device features.
    private boolean mEnableSupportedHardwareAcceleratedFeatures;
    private int mMixedContentMode = WebSettings.MIXED_CONTENT_NEVER_ALLOW;
    private boolean mCSSHexAlphaColorEnabled;
    private boolean mScrollTopLeftInteropEnabled;
    private boolean mWillSuppressErrorPage;

    private boolean mOffscreenPreRaster;
    private int mDisabledMenuItems = WebSettings.MENU_ITEM_NONE;

    // Although this bit is stored on AwSettings it is actually controlled via the
    // CookieManager.
    private boolean mAcceptThirdPartyCookies;


    private final boolean mSupportLegacyQuirks;
    private final boolean mAllowEmptyDocumentPersistence;
    private final boolean mAllowGeolocationOnInsecureOrigins;
    private final boolean mDoNotUpdateSelectionOnMutatingSelectionRange;

    private final boolean mPasswordEchoEnabled;

    // Not accessed by the native side.
    private boolean mBlockNetworkLoads; // Default depends on permission of embedding APK.
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

    static class LazyDefaultUserAgent {
        // Lazy Holder pattern
        private static final String sInstance = BvSettingsJni.get().getDefaultUserAgent();
    }

    // Protects access to settings global fields.
    private static final Object sGlobalContentSettingsLock = new Object();
    // For compatibility with the legacy WebView, we can only enable AppCache when
    // the path is
    // provided. However, we don't use the path, so we just check if we have
    // received it from the
    // client.
    private static boolean sAppCachePathIsSet;

    // The native side of this object. It's lifetime is bounded by the WebContent it
    // is attached to.
    private long mNativeBvSettings;

    // Custom handler that queues messages to call native code on the UI thread.
    private final EventHandler mEventHandler;

    private static final int MINIMUM_FONT_SIZE = 1;
    private static final int MAXIMUM_FONT_SIZE = 72;

    // Class to handle messages to be processed on the UI thread.
    private class EventHandler {
        // Message id for running a Runnable with mSettingsLock held.
        private static final int RUN_RUNNABLE_BLOCKING = 0;
        // Actual UI thread handler
        private Handler mHandler;
        // Synchronization flag.
        private boolean mSynchronizationPending;

        EventHandler() {
        }

        @SuppressLint("HandlerLeak")
        void bindUiThread() {
            if (mHandler != null)
                return;
            mHandler = new Handler(ThreadUtils.getUiThreadLooper()) {
                @Override
                public void handleMessage(Message msg) {
                    switch (msg.what) {
                        case RUN_RUNNABLE_BLOCKING:
                            synchronized (mSettingsLock) {
                                if (mNativeBvSettings != 0) {
                                    ((Runnable) msg.obj).run();
                                }
                                mSynchronizationPending = false;
                                mSettingsLock.notifyAll();
                            }
                            break;
                    }
                }
            };
        }

        void runOnUiThreadBlockingAndLocked(Runnable r) {
            assert Thread.holdsLock(mSettingsLock);
            if (mHandler == null)
                return;
            if (ThreadUtils.runningOnUiThread()) {
                r.run();
            } else {
                assert !mSynchronizationPending;
                mSynchronizationPending = true;
                mHandler.sendMessage(Message.obtain(null, RUN_RUNNABLE_BLOCKING, r));
                try {
                    while (mSynchronizationPending) {
                        mSettingsLock.wait();
                    }
                } catch (InterruptedException e) {
                    Log.e(LOGTAG, "Interrupted waiting a Runnable to complete", e);
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
        void onGestureZoomSupportChanged(
                boolean supportsDoubleTapZoom, boolean supportsMultiTouchZoom);
    }

    public BvSettings(Context context) {
        mContext = context;
        synchronized (mSettingsLock) {
            mHasInternetPermission = true;
            mBlockNetworkLoads = false;
            mEventHandler = new EventHandler();
            mUserAgent = LazyDefaultUserAgent.sInstance;

            // Best-guess a sensible initial value based on the features supported on the
            // device.
            mSpatialNavigationEnabled = !context.getPackageManager().hasSystemFeature(
                    PackageManager.FEATURE_TOUCHSCREEN);

            // Respect the system setting for password echoing.
            mPasswordEchoEnabled = Settings.System.getInt(context.getContentResolver(),
                    Settings.System.TEXT_SHOW_PASSWORD, 1) == 1;

            // By default, scale the text size by the system font scale factor. Embedders
            // may override this by invoking setTextZoom().
            mTextSizePercent = (int) (mTextSizePercent * context.getResources().getConfiguration().fontScale);

            mSupportLegacyQuirks = false;

            mAllowEmptyDocumentPersistence = false;
            mAllowGeolocationOnInsecureOrigins = false;
            mDoNotUpdateSelectionOnMutatingSelectionRange = false;

            mRequestedWithHeaderAllowedOriginRules = Collections.emptySet();
        }
        // Defer initializing the native side until a native WebContents instance is set.
    }

    public int getUiModeNight() {
        return mContext.getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK;
    }

    @CalledByNative
    private void nativeBvSettingsGone(long nativeBvSettings) {
        assert mNativeBvSettings != 0 && mNativeBvSettings == nativeBvSettings;
        mNativeBvSettings = 0;
    }

    @CalledByNative
    private double getDIPScaleLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mDIPScale;
    }

    void setDIPScale(double dipScale) {
        synchronized (mSettingsLock) {
            mDIPScale = dipScale;
        }
    }

    void setZoomListener(ZoomSupportChangeListener zoomChangeListener) {
        synchronized (mSettingsLock) {
            mZoomChangeListener = zoomChangeListener;
        }
    }

    void setWebContents(WebContents webContents) {
        synchronized (mSettingsLock) {
            if (mNativeBvSettings != 0) {
                BvSettingsJni.get().destroy(mNativeBvSettings, this);
                assert mNativeBvSettings == 0; // nativeAwSettingsGone should have been called.
            }
            if (webContents != null) {
                mEventHandler.bindUiThread();
                mNativeBvSettings = BvSettingsJni.get().init(this, webContents);
                updateEverythingLocked();
            }
        }
    }

    private void updateEverythingLocked() {
        assert Thread.holdsLock(mSettingsLock);
        assert mNativeBvSettings != 0;
        BvSettingsJni.get().updateEverythingLocked(mNativeBvSettings, this);
        onGestureZoomSupportChanged(
                supportsDoubleTapZoomLocked(), supportsMultiTouchZoomLocked());
    }

    /**
     * See {@link android.webkit.WebSettings#setBlockNetworkLoads}.
     */
    public void setBlockNetworkLoads(boolean flag) {
        if (TRACE)
            Log.i(LOGTAG, "setBlockNetworkLoads=" + flag);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return mBlockNetworkLoads;
        }
    }

    /**
     * Enable/disable third party cookies for an AwContents
     *
     * @param accept true if we should accept third party cookies
     */
    public void setAcceptThirdPartyCookies(boolean accept) {
        if (TRACE)
            Log.i(LOGTAG, "setAcceptThirdPartyCookies=" + accept);
        synchronized (mSettingsLock) {
            mAcceptThirdPartyCookies = accept;
            mEventHandler.updateCookiePolicyLocked();
        }
    }


    /**
     * Return whether third party cookies are enabled for an AwContents
     *
     * @return true if accept third party cookies
     */
    public boolean getAcceptThirdPartyCookies() {
        synchronized (mSettingsLock) {
            return mAcceptThirdPartyCookies;
        }
    }

    @CalledByNative
    private boolean getAcceptThirdPartyCookiesLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mAcceptThirdPartyCookies;
    }

    /**
     * Return whether Safe Browsing has been enabled for the current WebView
     *
     * @return true if SafeBrowsing is enabled
     */
    public boolean getSafeBrowsingEnabled() {
        synchronized (mSettingsLock) {
            return false;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setAllowFileAccess}.
     */
    public void setAllowFileAccess(boolean allow) {
        if (TRACE)
            Log.i(LOGTAG, "setAllowFileAccess=" + allow);
        synchronized (mSettingsLock) {
            mAllowFileUrlAccess = allow;
            mEventHandler.updateAllowFileAccessLocked();
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getAllowFileAccess}.
     */
    @CalledByNative
    public boolean getAllowFileAccess() {
        synchronized (mSettingsLock) {
            return mAllowFileUrlAccess;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setAllowContentAccess}.
     */
    public void setAllowContentAccess(boolean allow) {
        if (TRACE)
            Log.i(LOGTAG, "setAllowContentAccess=" + allow);
        synchronized (mSettingsLock) {
            mAllowContentUrlAccess = allow;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getAllowContentAccess}.
     */
    public boolean getAllowContentAccess() {
        synchronized (mSettingsLock) {
            return mAllowContentUrlAccess;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setCacheMode}.
     */
    public void setCacheMode(int mode) {
        if (TRACE)
            Log.i(LOGTAG, "setCacheMode=" + mode);
        synchronized (mSettingsLock) {
            mCacheMode = mode;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getCacheMode}.
     */
    public int getCacheMode() {
        synchronized (mSettingsLock) {
            return mCacheMode;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setNeedInitialFocus}.
     */
    public void setShouldFocusFirstNode(boolean flag) {
        if (TRACE)
            Log.i(LOGTAG, "setNeedInitialFocusNode=" + flag);
        synchronized (mSettingsLock) {
            mShouldFocusFirstNode = flag;
        }
    }

    /**
     * See {@link android.webkit.WebView#setInitialScale}.
     */
    public void setInitialPageScale(final float scaleInPercent) {
        if (TRACE)
            Log.i(LOGTAG, "setInitialScale=" + scaleInPercent);
        synchronized (mSettingsLock) {
            if (mInitialPageScalePercent != scaleInPercent) {
                mInitialPageScalePercent = scaleInPercent;
                mEventHandler.runOnUiThreadBlockingAndLocked(() -> {
                    if (mNativeBvSettings != 0) {
                        BvSettingsJni.get().updateInitialPageScaleLocked(
                                mNativeBvSettings, this);
                    }
                });
            }
        }
    }

    @CalledByNative
    private float getInitialPageScalePercentLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mInitialPageScalePercent;
    }

    void setSpatialNavigationEnabled(boolean enable) {
        synchronized (mSettingsLock) {
            if (mSpatialNavigationEnabled != enable) {
                mSpatialNavigationEnabled = enable;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    @CalledByNative
    private boolean getSpatialNavigationLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mSpatialNavigationEnabled;
    }

    void setEnableSupportedHardwareAcceleratedFeatures(boolean enable) {
        synchronized (mSettingsLock) {
            if (mEnableSupportedHardwareAcceleratedFeatures != enable) {
                mEnableSupportedHardwareAcceleratedFeatures = enable;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    @CalledByNative
    private boolean getEnableSupportedHardwareAcceleratedFeaturesLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mEnableSupportedHardwareAcceleratedFeatures;
    }

    public void setFullscreenSupported(boolean supported) {
        synchronized (mSettingsLock) {
            if (mFullscreenSupported != supported) {
                mFullscreenSupported = supported;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    @CalledByNative
    private boolean getFullscreenSupportedLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mFullscreenSupported;
    }

    /**
     * See {@link android.webkit.WebSettings#setNeedInitialFocus}.
     */
    public boolean shouldFocusFirstNode() {
        synchronized (mSettingsLock) {
            return mShouldFocusFirstNode;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setGeolocationEnabled}.
     */
    public void setGeolocationEnabled(boolean flag) {
        if (TRACE)
            Log.i(LOGTAG, "setGeolocationEnabled=" + flag);
        synchronized (mSettingsLock) {
            mGeolocationEnabled = flag;
        }
    }

    /**
     * @return Returns if geolocation is currently enabled.
     */
    boolean getGeolocationEnabled() {
        synchronized (mSettingsLock) {
            return mGeolocationEnabled;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setSaveFormData}.
     */
    public void setSaveFormData(final boolean enable) {
        if (TRACE)
            Log.i(LOGTAG, "setSaveFormData=" + enable);
        synchronized (mSettingsLock) {
            if (mAutoCompleteEnabled != enable) {
                mAutoCompleteEnabled = enable;
                mEventHandler.runOnUiThreadBlockingAndLocked(() -> {
                    if (mNativeBvSettings != 0) {
                        BvSettingsJni.get().updateFormDataPreferencesLocked(
                                mNativeBvSettings, this);
                    }
                });
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getSaveFormData}.
     */
    public boolean getSaveFormData() {
        synchronized (mSettingsLock) {
            return getSaveFormDataLocked();
        }
    }

    @CalledByNative
    private boolean getSaveFormDataLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mAutoCompleteEnabled;
    }

    public void setUserAgent(int ua) {
        // Minimal implementation for backwards compatibility: just supports resetting
        // to default.
        if (ua == 0) {
            setUserAgentString(null);
        } else {
            Log.w(LOGTAG, "setUserAgent not supported, ua=" + ua);
        }
    }

    /**
     * @returns the default User-Agent used by each WebContents instance, i.e.
     *          unless
     *          overridden by {@link #setUserAgentString(String ua)}
     */
    public static String getDefaultUserAgent() {
        return LazyDefaultUserAgent.sInstance;
    }

    @CalledByNative
    private static boolean getAllowSniffingFileUrls() {
        // Don't allow sniffing file:// URLs for MIME type if the application targets P
        // or later.
        return ContextUtils.getApplicationContext().getApplicationInfo().targetSdkVersion < Build.VERSION_CODES.P;
    }

    /**
     * See {@link android.webkit.WebSettings#setUserAgentString}.
     */
    public void setUserAgentString(String ua) {
        if (TRACE)
            Log.i(LOGTAG, "setUserAgentString=" + ua);
        synchronized (mSettingsLock) {
            final String oldUserAgent = mUserAgent;
            if (ua == null || ua.length() == 0) {
                mUserAgent = LazyDefaultUserAgent.sInstance;
            } else {
                mUserAgent = ua;
            }
            if (!oldUserAgent.equals(mUserAgent)) {
                mEventHandler.runOnUiThreadBlockingAndLocked(() -> {
                    if (mNativeBvSettings != 0) {
                        BvSettingsJni.get().updateUserAgentLocked(
                                mNativeBvSettings, this);
                    }
                });
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getUserAgentString}.
     */
    public String getUserAgentString() {
        synchronized (mSettingsLock) {
            return getUserAgentLocked();
        }
    }

    @CalledByNative
    private String getUserAgentLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mUserAgent;
    }

    /**
     * See {@link android.webkit.WebSettings#setLoadWithOverviewMode}.
     */
    public void setLoadWithOverviewMode(boolean overview) {
        if (TRACE)
            Log.i(LOGTAG, "setLoadWithOverviewMode=" + overview);
        synchronized (mSettingsLock) {
            if (mLoadWithOverviewMode != overview) {
                mLoadWithOverviewMode = overview;
                mEventHandler.runOnUiThreadBlockingAndLocked(() -> {
                    if (mNativeBvSettings != 0) {
                        updateWebkitPreferencesOnUiThreadLocked();
                        BvSettingsJni.get().resetScrollAndScaleState(
                                mNativeBvSettings, this);
                    }
                });
            }
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getLoadWithOverviewMode}.
     */
    public boolean getLoadWithOverviewMode() {
        synchronized (mSettingsLock) {
            return getLoadWithOverviewModeLocked();
        }
    }

    @CalledByNative
    private boolean getLoadWithOverviewModeLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mLoadWithOverviewMode;
    }

    /**
     * See {@link android.webkit.WebSettings#setTextZoom}.
     */
    public void setTextZoom(final int textZoom) {
        if (TRACE)
            Log.i(LOGTAG, "setTextZoom=" + textZoom);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return getTextSizePercentLocked();
        }
    }

    @CalledByNative
    private int getTextSizePercentLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mTextSizePercent;
    }

    /**
     * See {@link android.webkit.WebSettings#setStandardFontFamily}.
     */
    public void setStandardFontFamily(String font) {
        if (TRACE)
            Log.i(LOGTAG, "setStandardFontFamily=" + font);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return getStandardFontFamilyLocked();
        }
    }

    @CalledByNative
    private String getStandardFontFamilyLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mStandardFontFamily;
    }

    /**
     * See {@link android.webkit.WebSettings#setFixedFontFamily}.
     */
    public void setFixedFontFamily(String font) {
        if (TRACE)
            Log.i(LOGTAG, "setFixedFontFamily=" + font);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return getFixedFontFamilyLocked();
        }
    }

    @CalledByNative
    private String getFixedFontFamilyLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mFixedFontFamily;
    }

    /**
     * See {@link android.webkit.WebSettings#setSansSerifFontFamily}.
     */
    public void setSansSerifFontFamily(String font) {
        if (TRACE)
            Log.i(LOGTAG, "setSansSerifFontFamily=" + font);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return getSansSerifFontFamilyLocked();
        }
    }

    @CalledByNative
    private String getSansSerifFontFamilyLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mSansSerifFontFamily;
    }

    /**
     * See {@link android.webkit.WebSettings#setSerifFontFamily}.
     */
    public void setSerifFontFamily(String font) {
        if (TRACE)
            Log.i(LOGTAG, "setSerifFontFamily=" + font);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return getSerifFontFamilyLocked();
        }
    }

    @CalledByNative
    private String getSerifFontFamilyLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mSerifFontFamily;
    }

    /**
     * See {@link android.webkit.WebSettings#setCursiveFontFamily}.
     */
    public void setCursiveFontFamily(String font) {
        if (TRACE)
            Log.i(LOGTAG, "setCursiveFontFamily=" + font);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return getCursiveFontFamilyLocked();
        }
    }

    @CalledByNative
    private String getCursiveFontFamilyLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mCursiveFontFamily;
    }

    /**
     * See {@link android.webkit.WebSettings#setFantasyFontFamily}.
     */
    public void setFantasyFontFamily(String font) {
        if (TRACE)
            Log.i(LOGTAG, "setFantasyFontFamily=" + font);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return getFantasyFontFamilyLocked();
        }
    }

    @CalledByNative
    private String getFantasyFontFamilyLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mFantasyFontFamily;
    }

    /**
     * See {@link android.webkit.WebSettings#setMinimumFontSize}.
     */
    public void setMinimumFontSize(int size) {
        if (TRACE)
            Log.i(LOGTAG, "setMinimumFontSize=" + size);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return getMinimumFontSizeLocked();
        }
    }

    @CalledByNative
    private int getMinimumFontSizeLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mMinimumFontSize;
    }

    /**
     * See {@link android.webkit.WebSettings#setMinimumLogicalFontSize}.
     */
    public void setMinimumLogicalFontSize(int size) {
        if (TRACE)
            Log.i(LOGTAG, "setMinimumLogicalFontSize=" + size);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return getMinimumLogicalFontSizeLocked();
        }
    }

    @CalledByNative
    private int getMinimumLogicalFontSizeLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mMinimumLogicalFontSize;
    }

    /**
     * See {@link android.webkit.WebSettings#setDefaultFontSize}.
     */
    public void setDefaultFontSize(int size) {
        if (TRACE)
            Log.i(LOGTAG, "setDefaultFontSize=" + size);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return getDefaultFontSizeLocked();
        }
    }

    @CalledByNative
    private int getDefaultFontSizeLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mDefaultFontSize;
    }

    /**
     * See {@link android.webkit.WebSettings#setDefaultFixedFontSize}.
     */
    public void setDefaultFixedFontSize(int size) {
        if (TRACE)
            Log.i(LOGTAG, "setDefaultFixedFontSize=" + size);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return getDefaultFixedFontSizeLocked();
        }
    }

    @CalledByNative
    private int getDefaultFixedFontSizeLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mDefaultFixedFontSize;
    }

    /**
     * See {@link android.webkit.WebSettings#setJavaScriptEnabled}.
     */
    public void setJavaScriptEnabled(boolean flag) {
        if (TRACE)
            Log.i(LOGTAG, "setJavaScriptEnabled=" + flag);
        synchronized (mSettingsLock) {
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
        if (TRACE)
            Log.i(LOGTAG, "setAllowUniversalAccessFromFileURLs=" + flag);
        synchronized (mSettingsLock) {
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
        if (TRACE)
            Log.i(LOGTAG, "setAllowFileAccessFromFileURLs=" + flag);
        synchronized (mSettingsLock) {
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
        if (TRACE)
            Log.i(LOGTAG, "setLoadsImagesAutomatically=" + flag);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return getLoadsImagesAutomaticallyLocked();
        }
    }

    @CalledByNative
    private boolean getLoadsImagesAutomaticallyLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mLoadsImagesAutomatically;
    }

    /**
     * See {@link android.webkit.WebSettings#setImagesEnabled}.
     */
    public void setImagesEnabled(boolean flag) {
        if (TRACE)
            Log.i(LOGTAG, "setBlockNetworkImage=" + flag);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return mImagesEnabled;
        }
    }

    @CalledByNative
    private boolean getImagesEnabledLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mImagesEnabled;
    }

    /**
     * See {@link android.webkit.WebSettings#getJavaScriptEnabled}.
     */
    public boolean getJavaScriptEnabled() {
        synchronized (mSettingsLock) {
            return mJavaScriptEnabled;
        }
    }

    @CalledByNative
    private boolean getJavaScriptEnabledLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mJavaScriptEnabled;
    }

    /**
     * See {@link android.webkit.WebSettings#getAllowUniversalAccessFromFileURLs}.
     */
    public boolean getAllowUniversalAccessFromFileURLs() {
        synchronized (mSettingsLock) {
            return getAllowUniversalAccessFromFileURLsLocked();
        }
    }

    @CalledByNative
    private boolean getAllowUniversalAccessFromFileURLsLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mAllowUniversalAccessFromFileURLs;
    }

    /**
     * See {@link android.webkit.WebSettings#getAllowFileAccessFromFileURLs}.
     */
    public boolean getAllowFileAccessFromFileURLs() {
        synchronized (mSettingsLock) {
            return getAllowFileAccessFromFileURLsLocked();
        }
    }

    @CalledByNative
    private boolean getAllowFileAccessFromFileURLsLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mAllowFileAccessFromFileURLs;
    }

    /**
     * See
     * {@link android.webkit.WebSettings#setJavaScriptCanOpenWindowsAutomatically}.
     */
    public void setJavaScriptCanOpenWindowsAutomatically(boolean flag) {
        if (TRACE)
            Log.i(LOGTAG, "setJavaScriptCanOpenWindowsAutomatically=" + flag);
        synchronized (mSettingsLock) {
            if (mJavaScriptCanOpenWindowsAutomatically != flag) {
                mJavaScriptCanOpenWindowsAutomatically = flag;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    /**
     * See
     * {@link android.webkit.WebSettings#getJavaScriptCanOpenWindowsAutomatically}.
     */
    public boolean getJavaScriptCanOpenWindowsAutomatically() {
        synchronized (mSettingsLock) {
            return getJavaScriptCanOpenWindowsAutomaticallyLocked();
        }
    }

    @CalledByNative
    private boolean getJavaScriptCanOpenWindowsAutomaticallyLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mJavaScriptCanOpenWindowsAutomatically;
    }

    /**
     * See {@link android.webkit.WebSettings#setLayoutAlgorithm}.
     */
    public void setLayoutAlgorithm(@LayoutAlgorithm int l) {
        if (TRACE)
            Log.i(LOGTAG, "setLayoutAlgorithm=" + l);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return mLayoutAlgorithm;
        }
    }

    public void setRequestedWithHeaderOriginAllowList(Set<String> allowedOriginRules) {
      // allowedOriginRules =
      // allowedOriginRules != null ? allowedOriginRules : Collections.emptySet();
      //           AwWebContentsMetricsRecorder.recordRequestedWithHeaderModeAPIUsage(allowedOriginRules);
      //   synchronized (mSettingsLock) {
      //     setRequestedWithHeaderOriginAllowListLocked(allowedOriginRules);
      //   }
    }

    private void setRequestedWithHeaderOriginAllowListLocked(final Set<String> allowedOriginRules) {
      assert Thread.holdsLock(mSettingsLock);
      if (mNativeBvSettings == 0) {
          return;
      }

      // Final set to be updated by the Runnable on the UI thread.
      final Set<String> rejectedRules = new HashSet<>();

      mEventHandler.runOnUiThreadBlockingAndLocked(() -> {
          String[] rejected = BvSettingsJni.get().updateXRequestedWithAllowListOriginMatcher(
              mNativeBvSettings, allowedOriginRules.toArray(new String[0]));
          rejectedRules.addAll(java.util.Arrays.asList(rejected));
      });

      if (!rejectedRules.isEmpty()) {
          throw new IllegalArgumentException("Malformed origin match rules: " + rejectedRules);
      }
      mRequestedWithHeaderAllowedOriginRules = allowedOriginRules;
  }

    public Set<String> getRequestedWithHeaderOriginAllowList() {
        synchronized (mSettingsLock) {
            return mRequestedWithHeaderAllowedOriginRules;
        }
    }

    /**
     * Gets whether Text Auto-sizing layout algorithm is enabled.
     *
     * @return true if Text Auto-sizing layout algorithm is enabled
     */
    @CalledByNative
    private boolean getTextAutosizingEnabledLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mLayoutAlgorithm == LAYOUT_ALGORITHM_TEXT_AUTOSIZING;
    }

    /**
     * See {@link android.webkit.WebSettings#setSupportMultipleWindows}.
     */
    public void setSupportMultipleWindows(boolean support) {
        if (TRACE)
            Log.i(LOGTAG, "setSupportMultipleWindows=" + support);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return mSupportMultipleWindows;
        }
    }

    @CalledByNative
    private boolean getSupportMultipleWindowsLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mSupportMultipleWindows;
    }

    @CalledByNative
    private boolean getCSSHexAlphaColorEnabledLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mCSSHexAlphaColorEnabled;
    }

    public void setCSSHexAlphaColorEnabled(boolean enabled) {
        synchronized (mSettingsLock) {
            if (mCSSHexAlphaColorEnabled != enabled) {
                mCSSHexAlphaColorEnabled = enabled;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    @CalledByNative
    private boolean getScrollTopLeftInteropEnabledLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mScrollTopLeftInteropEnabled;
    }

    public void setScrollTopLeftInteropEnabled(boolean enabled) {
        synchronized (mSettingsLock) {
            if (mScrollTopLeftInteropEnabled != enabled) {
                mScrollTopLeftInteropEnabled = enabled;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    @CalledByNative
    private boolean getWillSuppressErrorPageLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mWillSuppressErrorPage;
    }

    public boolean getWillSuppressErrorPage() {
        synchronized (mSettingsLock) {
            return getWillSuppressErrorPageLocked();
        }
    }

    public void setWillSuppressErrorPage(boolean suppressed) {
        synchronized (mSettingsLock) {
            if (mWillSuppressErrorPage == suppressed)
                return;

            mWillSuppressErrorPage = suppressed;
            updateWillSuppressErrorStateLocked();
        }
    }

    private void updateWillSuppressErrorStateLocked() {
        mEventHandler.runOnUiThreadBlockingAndLocked(() -> {
            assert Thread.holdsLock(mSettingsLock);
            assert mNativeBvSettings != 0;
            BvSettingsJni.get().updateWillSuppressErrorStateLocked(
                    mNativeBvSettings, this);
        });
    }

    @CalledByNative
    private boolean getSupportLegacyQuirksLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mSupportLegacyQuirks;
    }

    @CalledByNative
    private boolean getAllowEmptyDocumentPersistenceLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mAllowEmptyDocumentPersistence;
    }

    @CalledByNative
    private boolean getAllowGeolocationOnInsecureOrigins() {
        assert Thread.holdsLock(mSettingsLock);
        return mAllowGeolocationOnInsecureOrigins;
    }

    @CalledByNative
    private boolean getDoNotUpdateSelectionOnMutatingSelectionRange() {
        assert Thread.holdsLock(mSettingsLock);
        return mDoNotUpdateSelectionOnMutatingSelectionRange;
    }

    /**
     * See {@link android.webkit.WebSettings#setUseWideViewPort}.
     */
    public void setUseWideViewPort(boolean use) {
        if (TRACE)
            Log.i(LOGTAG, "setUseWideViewPort=" + use);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return getUseWideViewportLocked();
        }
    }

    @CalledByNative
    private boolean getUseWideViewportLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mUseWideViewport;
    }

    public void setZeroLayoutHeightDisablesViewportQuirk(boolean enabled) {
        synchronized (mSettingsLock) {
            if (mZeroLayoutHeightDisablesViewportQuirk != enabled) {
                mZeroLayoutHeightDisablesViewportQuirk = enabled;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    public boolean getZeroLayoutHeightDisablesViewportQuirk() {
        synchronized (mSettingsLock) {
            return getZeroLayoutHeightDisablesViewportQuirkLocked();
        }
    }

    @CalledByNative
    private boolean getZeroLayoutHeightDisablesViewportQuirkLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mZeroLayoutHeightDisablesViewportQuirk;
    }

    public void setForceZeroLayoutHeight(boolean enabled) {
        synchronized (mSettingsLock) {
            if (mForceZeroLayoutHeight != enabled) {
                mForceZeroLayoutHeight = enabled;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    public boolean getForceZeroLayoutHeight() {
        synchronized (mSettingsLock) {
            return getForceZeroLayoutHeightLocked();
        }
    }

    @CalledByNative
    private boolean getForceZeroLayoutHeightLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mForceZeroLayoutHeight;
    }

    @CalledByNative
    private boolean getPasswordEchoEnabledLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mPasswordEchoEnabled;
    }

    /**
     * See {@link android.webkit.WebSettings#setDomStorageEnabled}.
     */
    public void setDomStorageEnabled(boolean flag) {
        if (TRACE)
            Log.i(LOGTAG, "setDomStorageEnabled=" + flag);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return mDomStorageEnabled;
        }
    }

    @CalledByNative
    private boolean getDomStorageEnabledLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mDomStorageEnabled;
    }

    /**
     * See {@link android.webkit.WebSettings#setDatabaseEnabled}.
     */
    public void setDatabaseEnabled(boolean flag) {
        if (TRACE)
            Log.i(LOGTAG, "setDatabaseEnabled=" + flag);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return mDatabaseEnabled;
        }
    }

    @CalledByNative
    private boolean getDatabaseEnabledLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mDatabaseEnabled;
    }

    /**
     * See {@link android.webkit.WebSettings#setDefaultTextEncodingName}.
     */
    public void setDefaultTextEncodingName(String encoding) {
        if (TRACE)
            Log.i(LOGTAG, "setDefaultTextEncodingName=" + encoding);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return getDefaultTextEncodingLocked();
        }
    }

    @CalledByNative
    private String getDefaultTextEncodingLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mDefaultTextEncoding;
    }

    /**
     * See {@link android.webkit.WebSettings#setMediaPlaybackRequiresUserGesture}.
     */
    public void setMediaPlaybackRequiresUserGesture(boolean require) {
        if (TRACE)
            Log.i(LOGTAG, "setMediaPlaybackRequiresUserGesture=" + require);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return getMediaPlaybackRequiresUserGestureLocked();
        }
    }

    @CalledByNative
    private boolean getMediaPlaybackRequiresUserGestureLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mMediaPlaybackRequiresUserGesture;
    }

    /**
     * See {@link android.webkit.WebSettings#setDefaultVideoPosterURL}.
     */
    public void setDefaultVideoPosterURL(String url) {
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return getDefaultVideoPosterURLLocked();
        }
    }

    @CalledByNative
    private String getDefaultVideoPosterURLLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mDefaultVideoPosterURL;
    }

    private void onGestureZoomSupportChanged(
            final boolean supportsDoubleTapZoom, final boolean supportsMultiTouchZoom) {
        // Always post asynchronously here, to avoid doubling back onto the caller.
        mEventHandler.maybePostOnUiThread(() -> {
            synchronized (mSettingsLock) {
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
        if (TRACE)
            Log.i(LOGTAG, "setSupportZoom=" + support);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return mSupportZoom;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setBuiltInZoomControls}.
     */
    public void setBuiltInZoomControls(boolean enabled) {
        if (TRACE)
            Log.i(LOGTAG, "setBuiltInZoomControls=" + enabled);
        synchronized (mSettingsLock) {
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
        synchronized (mSettingsLock) {
            return mBuiltInZoomControls;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#setDisplayZoomControls}.
     */
    public void setDisplayZoomControls(boolean enabled) {
        if (TRACE)
            Log.i(LOGTAG, "setDisplayZoomControls=" + enabled);
        synchronized (mSettingsLock) {
            mDisplayZoomControls = enabled;
        }
    }

    /**
     * See {@link android.webkit.WebSettings#getDisplayZoomControls}.
     */
    public boolean getDisplayZoomControls() {
        synchronized (mSettingsLock) {
            return mDisplayZoomControls;
        }
    }

    public void setMixedContentMode(int mode) {
        synchronized (mSettingsLock) {
            if (mMixedContentMode != mode) {
                mMixedContentMode = mode;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    @CalledByNative
    public int getMixedContentMode() {
        synchronized (mSettingsLock) {
            return mMixedContentMode;
        }
    }

    @ForceDarkMode
    public int getForceDarkMode() {
        synchronized (mSettingsLock) {
            return getForceDarkModeLocked();
        }
    }

    @CalledByNative
    @ForceDarkMode
    public int getForceDarkModeLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mForceDarkMode;
    }

    public void setForceDarkMode(@ForceDarkMode int forceDarkMode) {
        synchronized (mSettingsLock) {
            if (mForceDarkMode != forceDarkMode) {
                mForceDarkMode = forceDarkMode;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    public boolean isAlgorithmicDarkeningAllowed() {
        synchronized (mSettingsLock) {
            return isAlgorithmicDarkeningAllowedLocked();
        }
    }

    @CalledByNative
    private boolean isAlgorithmicDarkeningAllowedLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mAlgorithmicDarkeningAllowed;
    }

    public void setAlgorithmicDarkeningAllowed(boolean allow) {
        synchronized (mSettingsLock) {
            if (mAlgorithmicDarkeningAllowed != allow) {
                mAlgorithmicDarkeningAllowed = allow;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    @ForceDarkBehavior
    public int getForceDarkBehavior() {
        synchronized (mSettingsLock) {
            return getForceDarkBehaviorLocked();
        }
    }

    @CalledByNative
    @ForceDarkBehavior
    public int getForceDarkBehaviorLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mForceDarkBehavior;
    }

    public void setForceDarkBehavior(@ForceDarkBehavior int forceDarkBehavior) {
        synchronized (mSettingsLock) {
            if (mForceDarkBehavior != forceDarkBehavior) {
                mForceDarkBehavior = forceDarkBehavior;
                mEventHandler.updateWebkitPreferencesLocked();
            }
        }
    }

    @CalledByNative
    private boolean getAllowRunningInsecureContentLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mMixedContentMode == WebSettings.MIXED_CONTENT_ALWAYS_ALLOW;
    }

    @CalledByNative
    private boolean getUseStricMixedContentCheckingLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mMixedContentMode == WebSettings.MIXED_CONTENT_NEVER_ALLOW;
    }

    public boolean getOffscreenPreRaster() {
        synchronized (mSettingsLock) {
            return getOffscreenPreRasterLocked();
        }
    }

    @CalledByNative
    private boolean getOffscreenPreRasterLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mOffscreenPreRaster;
    }

    /**
     * 设置此WebView在屏幕外但附加到窗口时是否应栅格化图块。
     * 启用此功能可以避免在屏幕上为屏幕外的WebView设置动画时呈现伪像。
     * In particular, insertVisualStateCallback requires this mode to function.
     * Offscreen WebViews in this mode uses more memory. Please follow
     * these guidelines to limit memory usage:
     * - Webview size should be not be larger than the device screen size.
     * - Limit simple mode to a small number of webviews. Use it for
     *   visible webviews and webviews about to be animated to visible.
     */
    public void setOffscreenPreRaster(boolean enabled) {
        synchronized (mSettingsLock) {
            if (enabled != mOffscreenPreRaster) {
                mOffscreenPreRaster = enabled;
                mEventHandler.runOnUiThreadBlockingAndLocked(() -> {
                    if (mNativeBvSettings != 0) {
                        BvSettingsJni.get().updateOffscreenPreRasterLocked(
                                mNativeBvSettings, this);
                    }
                });
            }
        }
    }

    public int getDisabledActionModeMenuItems() {
        synchronized (mSettingsLock) {
            return mDisabledMenuItems;
        }
    }

    public void setDisabledActionModeMenuItems(int menuItems) {
        synchronized (mSettingsLock) {
            mDisabledMenuItems = menuItems;
        }
    }

    public void updateAcceptLanguages() {
        synchronized (mSettingsLock) {
            mEventHandler.runOnUiThreadBlockingAndLocked(() -> {
                if (mNativeBvSettings != 0) {
                    BvSettingsJni.get().updateRendererPreferencesLocked(
                            mNativeBvSettings, this);
                }
            });
        }
    }

    @CalledByNative
    private boolean supportsDoubleTapZoomLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mSupportZoom && mBuiltInZoomControls && mUseWideViewport;
    }

    private boolean supportsMultiTouchZoomLocked() {
        assert Thread.holdsLock(mSettingsLock);
        return mSupportZoom && mBuiltInZoomControls;
    }

    boolean supportsMultiTouchZoom() {
        synchronized (mSettingsLock) {
            return supportsMultiTouchZoomLocked();
        }
    }

    boolean shouldDisplayZoomControls() {
        synchronized (mSettingsLock) {
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
        assert Thread.holdsLock(mSettingsLock);
        // jiang set false
        return false;
        // return AwContentsStatics.getRecordFullDocument();
    }

    @CalledByNative
    private void updateEverything() {
        synchronized (mSettingsLock) {
            updateEverythingLocked();
        }
    }

    @CalledByNative
    private void populateWebPreferences(long webPrefsPtr) {
        synchronized (mSettingsLock) {
            assert mNativeBvSettings != 0;
            BvSettingsJni.get().populateWebPreferencesLocked(
                    mNativeBvSettings, this, webPrefsPtr);
        }
    }

    private void updateWebkitPreferencesOnUiThreadLocked() {
        assert mEventHandler.mHandler != null;
        ThreadUtils.assertOnUiThread();
        if (mNativeBvSettings != 0) {
            BvSettingsJni.get().updateWebkitPreferencesLocked(mNativeBvSettings, this);
        }
    }

    private void updateCookiePolicyOnUiThreadLocked() {
        assert mEventHandler.mHandler != null;
        ThreadUtils.assertOnUiThread();
        if (mNativeBvSettings != 0) {
            BvSettingsJni.get().updateCookiePolicyLocked(mNativeBvSettings, this);
        }
    }

    private void updateAllowFileAccessOnUiThreadLocked() {
        assert mEventHandler.mHandler != null;
        ThreadUtils.assertOnUiThread();
        if (mNativeBvSettings != 0) {
            BvSettingsJni.get().updateAllowFileAccessLocked(mNativeBvSettings, this);
        }
    }

    @NativeMethods
    interface Natives {
        long init(BvSettings caller, WebContents webContents);

        void destroy(long nativeBvSettings, BvSettings caller);

        void populateWebPreferencesLocked(
                long nativeBvSettings, BvSettings caller, long webPrefsPtr);

        void resetScrollAndScaleState(long nativeBvSettings, BvSettings caller);

        void updateEverythingLocked(long nativeBvSettings, BvSettings caller);

        void updateInitialPageScaleLocked(long nativeBvSettings, BvSettings caller);

        void updateUserAgentLocked(long nativeBvSettings, BvSettings caller);

        void updateWebkitPreferencesLocked(long nativeBvSettings, BvSettings caller);

        String getDefaultUserAgent();

        void updateFormDataPreferencesLocked(long nativeBvSettings, BvSettings caller);

        void updateRendererPreferencesLocked(long nativeBvSettings, BvSettings caller);

        void updateOffscreenPreRasterLocked(long nativeBvSettings, BvSettings caller);

        void updateWillSuppressErrorStateLocked(long nativeBvSettings, BvSettings caller);

        void updateCookiePolicyLocked(long nativeBvSettings, BvSettings caller);

        void updateAllowFileAccessLocked(long nativeBvSettings, BvSettings caller);

        String[] updateXRequestedWithAllowListOriginMatcher(long nativeBvSettings, String[] rules);
    }
}
