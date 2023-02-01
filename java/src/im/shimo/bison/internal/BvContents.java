package im.shimo.bison.internal;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ComponentCallbacks2;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;
import android.graphics.Picture;
import android.graphics.Rect;
import android.net.Uri;
import android.net.http.SslCertificate;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.text.TextUtils;
import android.util.Base64;
import android.util.Pair;
import android.util.SparseArray;
import android.view.DragEvent;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStructure;
import android.view.accessibility.AccessibilityNodeProvider;
import android.view.animation.AnimationUtils;
import android.view.autofill.AutofillValue;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.textclassifier.TextClassifier;
import android.webkit.JavascriptInterface;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import androidx.annotation.VisibleForTesting;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.LocaleUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.CalledByNativeUnchecked;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.metrics.ScopedSysTraceEvent;
import org.chromium.base.task.AsyncTask;
import org.chromium.base.task.PostTask;
import org.chromium.components.autofill.AutofillProvider;
import org.chromium.components.embedder_support.util.WebResourceResponseInfo;
import org.chromium.components.navigation_interception.InterceptNavigationDelegate;
import org.chromium.components.url_formatter.UrlFormatter;
import org.chromium.content_public.browser.ChildProcessImportance;
import org.chromium.content_public.browser.ImeAdapter;
import org.chromium.content_public.browser.ImeEventObserver;
import org.chromium.content_public.browser.JavaScriptCallback;
import org.chromium.content_public.browser.JavascriptInjector;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.MessagePayload;
import org.chromium.content_public.browser.MessagePort;
import org.chromium.content_public.browser.NavigationController;
import org.chromium.content_public.browser.NavigationHandle;
import org.chromium.content_public.browser.NavigationHistory;
import org.chromium.content_public.browser.RenderFrameHost;
import org.chromium.content_public.browser.SelectionClient;
import org.chromium.content_public.browser.SelectionPopupController;
import org.chromium.content_public.browser.SmartClipProvider;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.content_public.browser.ViewEventSink;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsAccessibility;
import org.chromium.content_public.browser.WebContentsInternals;
import org.chromium.content_public.browser.navigation_controller.LoadURLType;
import org.chromium.content_public.browser.navigation_controller.UserAgentOverrideOption;
import org.chromium.content_public.common.ContentUrlConstants;
import org.chromium.content_public.common.Referrer;
import org.chromium.net.NetworkChangeNotifier;
import org.chromium.network.mojom.ReferrerPolicy;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.Clipboard;
import org.chromium.ui.base.IntentRequestTracker;
import org.chromium.ui.base.PageTransition;
import org.chromium.ui.base.ViewAndroidDelegate;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.display.DisplayAndroid.DisplayAndroidObserver;
import org.chromium.url.GURL;

import java.io.File;
import java.lang.annotation.Annotation;
import java.lang.ref.WeakReference;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.WeakHashMap;
import java.util.concurrent.Callable;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import im.shimo.bison.BisonViewAndroidDelegate;
import im.shimo.bison.BvGeolocationCallback;
import im.shimo.bison.CleanupReference;
import im.shimo.bison.ContentViewRenderView;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
@JNINamespace("bison")
public class BvContents implements SmartClipProvider {
    private static final String TAG = "BvContents";
    private static final boolean TRACE = false;
    private static final int NO_WARN = 0;
    private static final int WARN = 1;
    private static final String PRODUCT_VERSION = BvContentsJni.get().getProductVersion();

    private static final String WEB_ARCHIVE_EXTENSION = ".mht";
    // The request code should be unique per BisonView/BvContents object.
    private static final int PROCESS_TEXT_REQUEST_CODE = 100;

    private static String sCurrentLocales = "";
    private static final float ZOOM_CONTROLS_EPSILON = 0.007f;

    private static final Pattern sFileAndroidAssetPattern = Pattern.compile("^file:/*android_(asset|res).*");

    private static final Pattern sDataURLWithSelectorPattern =
            Pattern.compile("^[^#]*(#[A-Za-z][A-Za-z0-9\\-_:.]*)$");

    /**
     * WebKit hit test related data structure. These are used to implement
     * getHitTestResult, requestFocusNodeHref, requestImageRef methods in WebView.
     * All values should be updated together. The native counterpart is
     * BvHitTestData.
     */
    public static class HitTestData {
        // Used in getHitTestResult.
        public int hitTestResultType;
        public String hitTestResultExtraData;

        // Used in requestFocusNodeHref (all three) and requestImageRef (only imgSrc).
        public String href;
        public String anchorText;
        public String imgSrc;
    }

    public interface InternalAccessDelegate extends ViewEventSink.InternalAccessDelegate {
        /**
         * @see View#overScrollBy(int, int, int, int, int, int, int, int, boolean);
         */
        void overScrollBy(int deltaX, int deltaY, int scrollX, int scrollY, int scrollRangeX, int scrollRangeY,
                int maxOverScrollX, int maxOverScrollY, boolean isTouchEvent);

        /**
         * @see View#scrollTo(int, int)
         */
        void super_scrollTo(int scrollX, int scrollY);

        /**
         * @see View#setMeasuredDimension(int, int)
         */
        void setMeasuredDimension(int measuredWidth, int measuredHeight);

        /**
         * @see View#getScrollBarStyle()
         */
        int super_getScrollBarStyle();

        /**
         * @see View#startActivityForResult(Intent, int)
         */
        void super_startActivityForResult(Intent intent, int requestCode);

        /**
         * @see View#onConfigurationChanged(Configuration)
         */
        void super_onConfigurationChanged(Configuration newConfig);
    }

    /**
     * Visual state callback, see {@link #insertVisualStateCallback} for details.
     *
     */
    @VisibleForTesting
    public abstract static class VisualStateCallback {
        /**
         * @param requestId the id passed to
         *                  {@link BvContents#insertVisualStateCallback} which can be
         *                  used to match requests with the corresponding callbacks.
         */
        public abstract void onComplete(long requestId);
    }

    private long mNativeBvContents;
    private BvBrowserContext mBrowserContext;
    private ViewGroup mContainerView;
    private Context mContext;
    private BisonViewAndroidDelegate mViewAndroidDelegate;
    private WindowAndroidWrapper mWindowAndroid;
    private WebContents mWebContents;
    private ViewEventSink mViewEventSink;
    private WebContentsInternalsHolder mWebContentsInternalsHolder;
    private NavigationController mNavigationController;
    private BvContentsClient mContentsClient;
    private BvWebContentsObserver mWebContentsObserver;
    private BvContentsClientBridge mBisonContentsClientBridge;
    private BvWebContentsDelegate mWebContentsDelegate;
    private final BvContentsBackgroundThreadClient mBackgroundThreadClient;
    private final BvContentsIoThreadClient mIoThreadClient;
    private final InterceptNavigationDelegateImpl mInterceptNavigationDelegate;
    private InternalAccessDelegate mInternalAccessAdapter;
    private final BvLayoutSizer mLayoutSizer;
    private final BvScrollOffsetManager mScrollOffsetManager;
    private OverScrollGlow mOverScrollGlow;
    private final DisplayAndroidObserver mDisplayObserver;
    private BvSettings mSettings;

    private boolean mIsPaused;
    private boolean mIsViewVisible;
    private boolean mIsWindowVisible;
    private boolean mIsAttachedToWindow;
    private boolean mIsContentVisible;
    private boolean mIsUpdateVisibilityTaskPending;
    private Runnable mUpdateVisibilityRunnable;

    private @RendererPriority int mRendererPriority;
    private boolean mRendererPriorityWaivedWhenNotVisible;

    private Bitmap mFavicon;

    private ContentViewRenderView mContentViewRenderView;
    private int mBaseBackgroundColor = Color.WHITE;

    private final HitTestData mPossiblyStaleHitTestData = new HitTestData();

    private boolean mContainerViewFocused;
    private boolean mWindowFocused;
    private float mPageScaleFactor = 1.0f;
    private float mMinPageScaleFactor = 1.0f;
    private float mMaxPageScaleFactor = 1.0f;
    private float mContentWidthDip;
    private float mContentHeightDip;

    private BvAutofillClient mAutofillClient;

    private BvPdfExporter mBvPdfExporter;

    private BvViewMethods mBvViewMethods;

    private boolean mTemporarilyDetached;
    private boolean mIsDestroyed;

    private AutofillProvider mAutofillProvider;
    private WebContentsInternals mWebContentsInternals;

    private JavascriptInjector mJavascriptInjector;

    private BvDarkMode mBvDarkMode;

    private static class WebContentsInternalsHolder implements WebContents.InternalsHolder {
        private final WeakReference<BvContents> mBisonContentsRef;

        private WebContentsInternalsHolder(BvContents bvContents) {
            mBisonContentsRef = new WeakReference<>(bvContents);
        }

        @Override
        public void set(WebContentsInternals internals) {
            BvContents bvContents = mBisonContentsRef.get();
            if (bvContents == null) {
                throw new IllegalStateException("BvContents should be available at this time");
            }
            bvContents.mWebContentsInternals = internals;
        }

        @Override
        public WebContentsInternals get() {
            BvContents bvContents = mBisonContentsRef.get();
            return bvContents == null ? null : bvContents.mWebContentsInternals;
        }

        public boolean weakRefCleared() {
            return mBisonContentsRef.get() == null;
        }
    }

    private static final class BisonContentsDestroyRunnable implements Runnable {
        private final long mNativeBvContents;
        // Hold onto a reference to the window (via its wrapper), so that it is not
        // destroyed
        // until we are done here.
        private final WindowAndroidWrapper mWindowAndroid;

        private BisonContentsDestroyRunnable(long nativeBvContents, WindowAndroidWrapper windowAndroid) {
            mNativeBvContents = nativeBvContents;
            mWindowAndroid = windowAndroid;
        }

        @Override
        public void run() {
            BvContentsJni.get().destroy(mNativeBvContents);
        }
    }

    private CleanupReference mCleanupReference;

    // --------------------------------------------------------------------------------------------
    private class IoThreadClientImpl extends BvContentsIoThreadClient {

        // All methods are called on the IO thread.

        @Override
        public int getCacheMode() {
            return mSettings.getCacheMode();
        }

        @Override
        public BvContentsBackgroundThreadClient getBackgroundThreadClient() {
            return mBackgroundThreadClient;
        }

        @Override
        public boolean shouldBlockContentUrls() {
            return !mSettings.getAllowContentAccess();
        }

        @Override
        public boolean shouldBlockFileUrls() {
            return !mSettings.getAllowFileAccess();
        }

        @Override
        public boolean shouldBlockNetworkLoads() {
            return mSettings.getBlockNetworkLoads();
        }

        @Override
        public boolean shouldAcceptThirdPartyCookies() {
            return mSettings.getAcceptThirdPartyCookies();
        }

        @Override
        public boolean getSafeBrowsingEnabled() {
            return mSettings.getSafeBrowsingEnabled();
        }
    }

    private class BackgroundThreadClientImpl extends BvContentsBackgroundThreadClient {
        // All methods are called on the background thread.

        @Override
        public WebResourceResponseInfo shouldInterceptRequest(BvWebResourceRequest request) {
            String url = request.url;
            WebResourceResponseInfo webResourceResponseInfo;
            // Return the response directly if the url is default video poster url.
            // webResourceResponse =
            // mDefaultVideoPosterRequestHandler.shouldInterceptRequest(url);
            // if (webResourceResponse != null) return webResourceResponse;

            webResourceResponseInfo = mContentsClient.shouldInterceptRequest(request);

            if (webResourceResponseInfo == null) {
                mContentsClient.getCallbackHelper().postOnLoadResource(url);
            }

            if (webResourceResponseInfo != null && webResourceResponseInfo.getData() == null) {
                // In this case the intercepted URLRequest job will simulate an empty response
                // which doesn't trigger the onReceivedError callback. For WebViewClassic
                // compatibility we synthesize that callback. http://crbug.com/180950
                mContentsClient.getCallbackHelper().postOnReceivedError(
                        request,
                        new BvContentsClient.BvWebResourceError());
            }
            return webResourceResponseInfo;
        }

        //jiang
        @Override
        public void overrideRequest(BvWebResourceRequest request) {
            mContentsClient.overrideRequest(request);
        }

    }

    private class InterceptNavigationDelegateImpl extends InterceptNavigationDelegate {
        @Override
        public boolean shouldIgnoreNavigation(NavigationHandle navigationHandle, GURL escapedUrl) {
            if (!navigationHandle.isRendererInitiated()) {
                GURL url = navigationHandle.getBaseUrlForDataUrl().isEmpty()
                        ? navigationHandle.getUrl()
                        : navigationHandle.getBaseUrlForDataUrl();
                mContentsClient.getCallbackHelper().postOnPageStarted(url.getPossiblyInvalidSpec());
            }
            return false;
        }
    }

    // --------------------------------------------------------------------------------------------
    private class LayoutSizerDelegate implements BvLayoutSizer.Delegate {
        @Override
        public void requestLayout() {
            if (mContainerView == null)
                return;
            mContainerView.requestLayout();
        }

        @Override
        public void setMeasuredDimension(int measuredWidth, int measuredHeight) {
            mInternalAccessAdapter.setMeasuredDimension(measuredWidth, measuredHeight);
        }

        @Override
        public boolean isLayoutParamsHeightWrapContent() {
            return mContainerView != null && mContainerView.getLayoutParams() != null
                    && (mContainerView.getLayoutParams().height == ViewGroup.LayoutParams.WRAP_CONTENT);
        }

        @Override
        public void setForceZeroLayoutHeight(boolean forceZeroHeight) {
            getSettings().setForceZeroLayoutHeight(forceZeroHeight);
        }
    }

    // --------------------------------------------------------------------------------------------
    private class BvScrollOffsetManagerDelegate implements BvScrollOffsetManager.Delegate {
        @Override
        public void overScrollContainerViewBy(int deltaX, int deltaY, int scrollX, int scrollY, int scrollRangeX,
                int scrollRangeY, boolean isTouchEvent) {
            mInternalAccessAdapter.overScrollBy(deltaX, deltaY, scrollX, scrollY, scrollRangeX, scrollRangeY, 0, 0,
                    isTouchEvent);
        }

        @Override
        public void scrollContainerViewTo(int x, int y) {
            mInternalAccessAdapter.super_scrollTo(x, y);
        }

        @Override
        public void scrollNativeTo(int x, int y) {
            // jiang
        }

        @Override
        public void smoothScroll(int targetX, int targetY, long durationMs) {
            // jiang
        }

        @Override
        public int getContainerViewScrollX() {
            if (mContainerView == null)
                return 0;
            return mContainerView.getScrollX();
        }

        @Override
        public int getContainerViewScrollY() {
            if (mContainerView == null)
                return 0;
            return mContainerView.getScrollY();
        }

        @Override
        public void invalidate() {
            postInvalidateOnAnimation();
        }

        @Override
        public void cancelFling() {
            mWebContents.getEventForwarder().cancelFling(SystemClock.uptimeMillis());
        }
    }

    // --------------------------------------------------------------------------------------------
    private class BisonDisplayAndroidObserver implements DisplayAndroidObserver {
        @Override
        public void onRotationChanged(int rotation) { }


        @Override
        public void onDIPScaleChanged(float dipScale) {
            if (TRACE)
                Log.i(TAG, "%s onDIPScaleChanged dipScale=%f", this, dipScale);

            BvContentsJni.get().setDipScale(mNativeBvContents, BvContents.this, dipScale);
            mLayoutSizer.setDIPScale(dipScale);
            mSettings.setDIPScale(dipScale);
        }
    }

    // --------------------------------------------------------------------------------------------
    public BvContents(Context context, ViewGroup containerView, BvBrowserContext bvBrowserContext,
            InternalAccessDelegate internalAccessAdapter, BvContentsClientBridge bvContentsClientBridge,
            BvContentsClient bvContentsClient, BvSettings settings, int webContentsRenderView) {
        mRendererPriority = RendererPriority.HIGH;
        mSettings = settings;
        updateDefaultLocale();

        mBrowserContext = bvBrowserContext;

        mNativeBvContents = BvContentsJni.get().init(this, mBrowserContext.getNativePointer());
        mWebContents = BvContentsJni.get().getWebContents(mNativeBvContents);
        mContainerView = containerView;
        mContext = context;
        mInternalAccessAdapter = internalAccessAdapter;
        mContentsClient = bvContentsClient;
        mBvViewMethods = new BvViewMethodsImpl();
        mLayoutSizer = new BvLayoutSizer();
        mLayoutSizer.setDelegate(new LayoutSizerDelegate());
        mWindowAndroid = getWindowAndroid(context);
        mViewAndroidDelegate = new BisonViewAndroidDelegate(containerView);
        mWebContentsInternalsHolder = new WebContentsInternalsHolder(this);
        mBisonContentsClientBridge = bvContentsClientBridge;
        // mAutofillProvider

        mBackgroundThreadClient = new BackgroundThreadClientImpl();
        mIoThreadClient = new IoThreadClientImpl();
        mInterceptNavigationDelegate = new InterceptNavigationDelegateImpl();
        mWebContentsDelegate = new BvWebContentsDelegate(context, this, mContentsClient, mSettings, mContainerView);
        mDisplayObserver = new BisonDisplayAndroidObserver();
        initWebContents(mViewAndroidDelegate, mInternalAccessAdapter, mWebContents, mWindowAndroid.getWindowAndroid(),
                mWebContentsInternalsHolder, webContentsRenderView);
        BvContentsJni.get().setJavaPeers(mNativeBvContents, mWebContentsDelegate, mBisonContentsClientBridge,
                mIoThreadClient, mInterceptNavigationDelegate, mAutofillProvider);

        installWebContentsObserver();
        mSettings.setWebContents(mWebContents);

        mDisplayObserver.onDIPScaleChanged(getDeviceScaleFactor());
        mBvDarkMode = new BvDarkMode(context);
        mBvDarkMode.setWebContents(mWebContents);
        mScrollOffsetManager = new BvScrollOffsetManager(new BvScrollOffsetManagerDelegate());

        mUpdateVisibilityRunnable = this::updateWebContentsVisibility;
        mCleanupReference = new CleanupReference(this,
                new BisonContentsDestroyRunnable(mNativeBvContents, mWindowAndroid));

    }

    private void initWebContents(ViewAndroidDelegate viewDelegate, InternalAccessDelegate internalDispatcher,
            WebContents webContents, WindowAndroid windowAndroid,
            WebContentsInternalsHolder internalsHolder, int webContentsRenderView) {
        mContentViewRenderView = new ContentViewRenderView(mContext);
        mContentViewRenderView.onNativeLibraryLoaded(windowAndroid, webContentsRenderView);
        // mWindowAndroid.getWindowAndroid().setAnimationPlaceholderView(mContentViewRenderView.getSurfaceView());
        mContainerView.addView(mContentViewRenderView);
        mWebContents.initialize(PRODUCT_VERSION, mViewAndroidDelegate, mInternalAccessAdapter,
                mWindowAndroid.getWindowAndroid(), mWebContentsInternalsHolder);
        mNavigationController = mWebContents.getNavigationController();

        mContentViewRenderView.setWebContents(mWebContents);
        mViewEventSink = ViewEventSink.from(mWebContents);
        mViewEventSink.setHideKeyboardOnBlur(false);
        SelectionPopupController controller = SelectionPopupController.fromWebContents(webContents);
        controller.setActionModeCallback(new BvActionModeCallback(this, mWebContents));
        // controller.setSelectionClient(SelectionClient.createSmartSelectionClient(webContents));
        ImeAdapter.fromWebContents(webContents).addEventObserver(new ImeEventObserver() {
            @Override
            public void onBeforeSendKeyEvent(KeyEvent event) {
                if (BvContents.isDpadEvent(event)) {
                    mSettings.setSpatialNavigationEnabled(true);
                }
            }
        });
    }

    private void setInternalAccessAdapter(InternalAccessDelegate internalAccessAdapter) {
        mInternalAccessAdapter = internalAccessAdapter;
        mViewEventSink.setAccessDelegate(mInternalAccessAdapter);
    }

    // This class destroys the WindowAndroid when after it is gc-ed.
    private static class WindowAndroidWrapper {
        private final WindowAndroid mWindowAndroid;
        private final CleanupReference mCleanupReference;

        private static final class DestroyRunnable implements Runnable {
            private final WindowAndroid mWindowAndroid;
            private DestroyRunnable(WindowAndroid windowAndroid) {
                mWindowAndroid = windowAndroid;
            }
            @Override
            public void run() {
                mWindowAndroid.destroy();
            }
        }

        public WindowAndroidWrapper(WindowAndroid windowAndroid) {
            try (ScopedSysTraceEvent e = ScopedSysTraceEvent.scoped("WindowAndroidWrapper.constructor")) {
                mWindowAndroid = windowAndroid;
                mCleanupReference = new CleanupReference(this, new DestroyRunnable(windowAndroid));
            }
        }

        public WindowAndroid getWindowAndroid() {
            return mWindowAndroid;
        }
    }
    // jiang webview 加个开关
    // private static WeakHashMap<Context, WindowAndroidWrapper> sContextWindowMap;

    // getWindowAndroid is only called on UI thread, so there are no threading
    // issues with lazy
    // initialization.
    private static WindowAndroidWrapper getWindowAndroid(Context context) {
        // if (sContextWindowMap == null) sContextWindowMap = new WeakHashMap<>();
        // WindowAndroidWrapper wrapper = sContextWindowMap.get(context);

        WindowAndroidWrapper wrapper = null;

        try (ScopedSysTraceEvent e = ScopedSysTraceEvent.scoped("BisonContents.getWindowAndroid")) {
            Activity activity = ContextUtils.activityFromContext(context);
            if (activity != null) {
                ActivityWindowAndroid activityWindow;
                try (ScopedSysTraceEvent e2 = ScopedSysTraceEvent.scoped("BisonContents.createActivityWindow")) {
                    final boolean listenToActivityState = false;
                    activityWindow = new ActivityWindowAndroid(context, listenToActivityState,
                            IntentRequestTracker.createFromActivity(activity));
                }
                wrapper = new WindowAndroidWrapper(activityWindow);
            } else {
                wrapper = new WindowAndroidWrapper(new WindowAndroid(context));
            }
            // sContextWindowMap.put(context, wrapper);
        }
        return wrapper;
    }

    public void updateDefaultLocale() {
        String locales = LocaleUtils.getDefaultLocaleListString();
        if (!sCurrentLocales.equals(locales)) {
            sCurrentLocales = locales;

            // We cannot use the first language in sCurrentLocales for the UI language even
            // on
            // Android N. LocaleUtils.getDefaultLocaleString() is capable for UI language
            // but
            // it is not guaranteed to be listed at the first of sCurrentLocales. Therefore,
            // both values are passed to native.
            BvContentsJni.get().updateDefaultLocale(
                    LocaleUtils.getDefaultLocaleString(), sCurrentLocales);
            mSettings.updateAcceptLanguages();
        }
    }

    private void setWebContents(long newBisonContentsPtr) {
        if (mNativeBvContents != 0) {
            destroyNatives();
            mWebContents = null;
            mWebContentsInternalsHolder = null;
            mWebContentsInternals = null;
            mNavigationController = null;
            mJavascriptInjector = null;
        }

    }

    private void installWebContentsObserver() {
        if (mWebContentsObserver != null) {
            mWebContentsObserver.destroy();
        }
        mWebContentsObserver = new BvWebContentsObserver(mWebContents, this, mContentsClient);
    }

    private JavascriptInjector getJavascriptInjector() {
        if (mJavascriptInjector == null) {
            mJavascriptInjector = JavascriptInjector.fromWebContents(mWebContents, false);
        }
        return mJavascriptInjector;
    }

    @CalledByNative
    private void onRendererResponsive(BvRenderProcess renderProcess) {
        if (isDestroyed(NO_WARN))
            return;
        mContentsClient.onRendererResponsive(renderProcess);
    }

    @CalledByNative
    private void onRendererUnresponsive(BvRenderProcess renderProcess) {
        if (isDestroyed(NO_WARN))
            return;
        mContentsClient.onRendererUnresponsive(renderProcess);
    }

    @CalledByNativeUnchecked
    protected boolean onRenderProcessGone(int childProcessID, boolean crashed) {
        if (isDestroyed(NO_WARN))
            return false;
        return mContentsClient.onRenderProcessGone(new BvRenderProcessGoneDetail(crashed,
                BvContentsJni.get().getEffectivePriority(mNativeBvContents, this)));
    }



    /**
     * Destroys this object and deletes its native counterpart.
     */
    public void destroy() {
        if (TRACE)
            Log.i(TAG, "%s destroy", this);
        if (isDestroyed(NO_WARN))
            return;
        if (mContentViewRenderView != null) {
            mContentViewRenderView.destroy();
            mContentViewRenderView = null;
        }
        mWebContents.setTopLevelNativeWindow(null);
        if (mAutofillClient != null) {
            mAutofillClient = null;
        }

        if (mBvDarkMode != null) {
            mBvDarkMode.destroy();
            mBvDarkMode = null;
        }

        // Remove pending messages
        mContentsClient.getCallbackHelper().removeCallbacksAndMessages();
        mIsDestroyed = true;
        PostTask.postTask(UiThreadTaskTraits.DEFAULT, () -> destroyNatives());
    }

    /**
     * Deletes the native counterpart of this object.
     */
    @VisibleForTesting
    public void destroyNatives() {
        if (mCleanupReference != null) {
            assert mNativeBvContents != 0;

            mWebContentsObserver.destroy();
            mWebContentsObserver = null;
            mNativeBvContents = 0;
            mWebContents = null;
            mWebContentsInternals = null;
            mNavigationController = null;

            mCleanupReference.cleanupNow();
            mCleanupReference = null;
        }
        if (mContentsClient != null) {
            mContentsClient = null;
        }
        if (mContext != null) {
            mContext = null;
        }
        if (mContainerView != null) {
            mContainerView = null;
        }
        if (mWebContentsDelegate != null) {
            mWebContentsDelegate = null;
        }
        if (mBisonContentsClientBridge != null) {
            mBisonContentsClientBridge = null;
        }
        if (mViewAndroidDelegate != null) {
            mViewAndroidDelegate.setContainerView(null);
            mViewAndroidDelegate = null;
        }
        if (mJavascriptInjector != null) {
            mJavascriptInjector = null;
        }

        assert mWebContents == null;
        assert mNavigationController == null;
        assert mNativeBvContents == 0;

        onDestroyed();
    }

    @VisibleForTesting
    protected void onDestroyed() {
    }

    private boolean isDestroyed(int warnIfDestroyed) {
        if (mIsDestroyed && warnIfDestroyed == WARN) {
            Log.w(TAG, "Application attempted to call on a destroyed BisonView", new Throwable());
        }
        boolean destroyRunnableHasRun = mCleanupReference != null && mCleanupReference.hasCleanedUp();
        boolean weakRefsCleared = mWebContentsInternalsHolder != null && mWebContentsInternalsHolder.weakRefCleared();
        if (TRACE && destroyRunnableHasRun && !mIsDestroyed) {
            // Swallow the error. App developers are not going to do anything with an error
            // msg.
            Log.d(TAG, "BisonContents is kept alive past CleanupReference by finalizer");
        }
        return mIsDestroyed || destroyRunnableHasRun || weakRefsCleared;
    }

    @VisibleForTesting
    public WebContents getWebContents() {
        return mWebContents;
    }

    @VisibleForTesting
    public NavigationController getNavigationController() {
        return mNavigationController;
    }

    public BvSettings getSettings() {
        return mSettings;
    }

    ViewGroup getContainerView() {
        return mContainerView;
    }

    public BvPdfExporter getPdfExporter() {
        if (isDestroyed(WARN))
            return null;
        if (mBvPdfExporter == null) {
            mBvPdfExporter = new BvPdfExporter();
            BvContentsJni.get().createPdfExporter(mNativeBvContents, mBvPdfExporter);
        }
        return mBvPdfExporter;
    }

    public static void setShouldDownloadFavicons() {
        BvContentsJni.get().setShouldDownloadFavicons();
    }

    /**
     * Disables contents of JS-to-Java bridge objects to be inspectable using
     * Object.keys() method and "for .. in" loops. This is intended for applications
     * targeting earlier Android releases where this was not possible, and we want
     * to ensure backwards compatible behavior.
     */
    public void disableJavascriptInterfacesInspection() {
        if (!isDestroyed(WARN)) {
            getJavascriptInjector().setAllowInspection(false);
        }
    }

    private static final Rect sLocalGlobalVisibleRect = new Rect();

    private Rect getGlobalVisibleRect() {
        if (mContainerView == null || !mContainerView.getGlobalVisibleRect(sLocalGlobalVisibleRect)) {
            sLocalGlobalVisibleRect.setEmpty();
        }
        return sLocalGlobalVisibleRect;
    }

    // jiang 暂时不实现
    public void onDraw(Canvas canvas) {
    }

    public void setLayoutParams(final ViewGroup.LayoutParams layoutParams) {
        mLayoutSizer.onLayoutParamsChange();
    }

    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        mBvViewMethods.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    public int getContentHeightCss() {
        if (isDestroyed(WARN))
            return 0;
        return (int) Math.ceil(mContentHeightDip);
    }

    public int getContentWidthCss() {
        if (isDestroyed(WARN))
            return 0;
        return (int) Math.ceil(mContentWidthDip);
    }

    public Picture capturePicture() {
        // jiang
        // if (TRACE) Log.i(TAG, "%s capturePicture", this);
        // if (isDestroyed(WARN)) return null;
        // return new AwPicture(AwContentsJni.get().capturePicture(mNativeBvContents,
        // BvContents.this,
        // mScrollOffsetManager.computeHorizontalScrollRange(),
        // mScrollOffsetManager.computeVerticalScrollRange()));
        return null;
    }

    // jiang
    // public void clearView() {
    // if (TRACE) Log.i(TAG, "%s clearView", this);
    // if (!isDestroyed(WARN)) AwContentsJni.get().clearView(mNativeBvContents,
    // BvContents.this);
    // }

    // jiang
    // public void enableOnNewPicture(boolean enabled, boolean invalidationOnly) {
    // }

    public void findAllAsync(String searchString) {
        if (TRACE) Log.i(TAG, "%s findAllAsync", this);
        if (isDestroyed(WARN)) return;
        if (searchString == null) {
            throw new IllegalArgumentException("Search string shouldn't be null");
        }
        BvContentsJni.get().findAllAsync(mNativeBvContents, searchString);
    }

    public void findNext(boolean forward) {
        if (TRACE)
            Log.i(TAG, "%s findNext", this);
        if (!isDestroyed(WARN)) {
            BvContentsJni.get().findNext(mNativeBvContents, forward);
        }
    }

    public void clearMatches() {
        if (TRACE)
            Log.i(TAG, "%s clearMatches", this);
        if (!isDestroyed(WARN)) {
            BvContentsJni.get().clearMatches(mNativeBvContents, BvContents.this);
        }
    }


    /**
     * @return load progress of the WebContents, on a scale of 0-100.
     */
    public int getMostRecentProgress() {
        if (isDestroyed(WARN)) return 0;
        if (!mWebContents.isLoading()) return 100;
        return Math.round(100 * mWebContents.getLoadProgress());
    }

    public Bitmap getFavicon() {
        if (isDestroyed(WARN))
            return null;
        return mFavicon;
    }

    /* package */ static final Pattern BAD_HEADER_CHAR = Pattern.compile("[\u0000\r\n]");
    /* package */ static final String BAD_HEADER_MSG =
    "HTTP headers must not contain null, CR, or NL characters. ";
    public void loadUrl(String url, Map<String, String> additionalHttpHeaders) {
        if (TRACE) Log.i(TAG, "%s loadUrl(extra headers)=%s", this, url);
        if (isDestroyed(WARN)) return;
        // Early out to match old WebView implementation
        if (url == null) {
            return;
        }
        // TODO: We may actually want to do some sanity checks here (like filter about://chrome).


        LoadUrlParams params = new LoadUrlParams(url, PageTransition.TYPED);
        if (additionalHttpHeaders != null) {
            for (Map.Entry<String, String> header : additionalHttpHeaders.entrySet()) {
                String headerName = header.getKey();
                String headerValue = header.getValue();
                if (headerName != null && BAD_HEADER_CHAR.matcher(headerName).find()) {
                    throw new IllegalArgumentException(
                            BAD_HEADER_MSG + "Invalid header name '" + headerName + "'.");
                }
                if (headerValue != null && BAD_HEADER_CHAR.matcher(headerValue).find()) {
                    throw new IllegalArgumentException(BAD_HEADER_MSG + "Header '" + headerName
                            + "' has invalid value '" + headerValue + "'");
                }
            }
            params.setExtraHeaders(new HashMap<String, String>(additionalHttpHeaders));
        }

        loadUrl(params);
    }

    public void loadUrl(String url) {
      if (TRACE) Log.i(TAG, "%s loadUrl=%s", this, url);
      if (isDestroyed(WARN)) return;
        if (url == null) {
          return;
        }
        loadUrl(url, null);
    }

    public void postUrl(String url, byte[] postData) {
        if (TRACE)
            Log.i(TAG, "%s postUrl=%s", this, url);
        if (isDestroyed(WARN))
            return;
        LoadUrlParams params = LoadUrlParams.createLoadHttpPostParams(url, postData);
        Map<String, String> headers = new HashMap<>();
        headers.put("Content-Type", "application/x-www-form-urlencoded");
        params.setExtraHeaders(headers);
        loadUrl(params);
    }

    private static String fixupMimeType(String mimeType) {
        return TextUtils.isEmpty(mimeType) ? "text/html" : mimeType;
    }

    private static String fixupData(String data) {
        return TextUtils.isEmpty(data) ? "" : data;
    }

    private static String fixupBase(String url) {
        return TextUtils.isEmpty(url) ? ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL : url;
    }

    private static String fixupHistory(String url) {
        return TextUtils.isEmpty(url) ? ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL : url;
    }

    private static boolean isBase64Encoded(String encoding) {
        return "base64".equals(encoding);
    }

    /*

     */
    public void loadData(String data, String mimeType, String encoding) {
        if (TRACE) Log.i(TAG, "%s loadData", this);
        if (isDestroyed(WARN)) return;
        if (data != null && data.contains("#")) {
            if (ContextUtils.getApplicationContext().getApplicationInfo().targetSdkVersion
                            < Build.VERSION_CODES.Q
                    && !isBase64Encoded(encoding)) {
                // As of Chromium M72, data URI parsing strictly enforces encoding of '#'. To
                // support WebView applications which were not expecting this change, we do it for
                // them.
                data = fixupOctothorpesInLoadDataContent(data);
            }
        }
        loadUrl(LoadUrlParams.createLoadDataParams(
                fixupData(data), fixupMimeType(mimeType), isBase64Encoded(encoding)));
    }

    public static String fixupOctothorpesInLoadDataContent(String data) {
        // If the data may have had a valid DOM selector, we duplicate the selector and append it as
        // a proper URL fragment. For example, "<a id='target'>Target</a>#target" will be converted
        // to "<a id='target'>Target</a>%23target#target". This preserves both the rendering (which
        // should render 'Target#target' on the page) and the DOM selector behavior (which should
        // scroll to the anchor).
        Matcher matcher = sDataURLWithSelectorPattern.matcher(data);
        String suffix = matcher.matches() ? matcher.group(1) : "";
        return data.replace("#", "%23") + suffix;
    }

    public void loadDataWithBaseURL(
            String baseUrl, String data, String mimeType, String encoding, String historyUrl) {
        if (TRACE)
            Log.i(TAG, "%s loadDataWithBaseURL=%s", this, baseUrl);
        if (isDestroyed(WARN))
            return;

        data = fixupData(data);
        mimeType = fixupMimeType(mimeType);
        LoadUrlParams loadUrlParams;
        baseUrl = fixupBase(baseUrl);
        historyUrl = fixupHistory(historyUrl);

        if (baseUrl.startsWith("data:")) {
            boolean isBase64 = isBase64Encoded(encoding);
            loadUrlParams = LoadUrlParams.createLoadDataParamsWithBaseUrl(
                    data, mimeType, isBase64, baseUrl, historyUrl, isBase64 ? null : encoding);
        } else {
            try {
                loadUrlParams = LoadUrlParams.createLoadDataParamsWithBaseUrl(
                        Base64.encodeToString(data.getBytes("utf-8"), Base64.DEFAULT), mimeType,
                        true, baseUrl, historyUrl, "utf-8");
            } catch (java.io.UnsupportedEncodingException e) {
                Log.wtf(TAG, "Unable to load data string %s", data, e);
                return;
            }
        }

        loadUrl(loadUrlParams);
    }

    public void loadUrl(LoadUrlParams params) {
        if (params.getLoadUrlType() == LoadURLType.DATA && !params.isBaseUrlDataScheme()) {
            params.setCanLoadLocalResources(true);
            BvContentsJni.get().grantFileSchemeAccesstoChildProcess(mNativeBvContents);
        }

        if (params.getUrl() != null
                && params.getUrl().equals(mWebContents.getLastCommittedUrl().getSpec())
                && params.getTransitionType() == PageTransition.TYPED) {
            params.setTransitionType(PageTransition.RELOAD);
        }
        params.setTransitionType(params.getTransitionType() | PageTransition.FROM_API);

        params.setOverrideUserAgent(UserAgentOverrideOption.TRUE);

        final String referer = "referer";
        Map<String, String> extraHeaders = params.getExtraHeaders();
        if (extraHeaders != null) {
            for (String header : extraHeaders.keySet()) {
                if (referer.equals(header.toLowerCase(Locale.US))) {
                    params.setReferrer(
                            new Referrer(extraHeaders.remove(header), ReferrerPolicy.DEFAULT));
                    params.setExtraHeaders(extraHeaders);
                    break;
                }
            }
        }

        BvContentsJni.get().setExtraHeadersForUrl(mNativeBvContents, params.getUrl(),
                params.getExtraHttpRequestHeadersString());
        params.setExtraHeaders(new HashMap<String, String>());

        params.setUrl(UrlFormatter.fixupUrl(params.getUrl()).getPossiblyInvalidSpec());

        mNavigationController.loadUrl(params);

        // if (!mHasRequestedVisitedHistoryFromClient) {
        // mHasRequestedVisitedHistoryFromClient = true;
        // requestVisitedHistoryFromClient();
        // }
    }

    /**
     * Get the URL of the current page. This is the visible URL of the
     * {@link WebContents} which may be a pending navigation or the last committed
     * URL. For the last committed URL use #getLastCommittedUrl().
     *
     * @return The URL of the current page or null if it's empty.
     */
    public GURL getUrl() {
        if (TRACE) Log.i(TAG, "%s getUrl", this);
        if (isDestroyed(WARN)) return null;
        GURL url = mWebContents.getVisibleUrl();
        if (url == null || url.getSpec().trim().isEmpty()) return null;
        return url;
    }

    /**
     * Gets the last committed URL. It represents the current page that is displayed
     * in WebContents. It represents the current security context.
     *
     * @return The URL of the current page or null if it's empty.
     */
    public String getLastCommittedUrl() {
        if (TRACE) Log.i(TAG, "%s getLastCommittedUrl", this);
        if (isDestroyed(NO_WARN)) return null;
        GURL url = mWebContents.getLastCommittedUrl();
        if (url == null || url.isEmpty()) return null;
        return url.getSpec();
    }

    public void requestFocus() {
        mBvViewMethods.requestFocus();
    }

    public void setBackgroundColor(int color) {
        mContentViewRenderView.setBackgroundColor(color);
        if (!isDestroyed(WARN)) {
            BvContentsJni.get().setBackgroundColor(mNativeBvContents, BvContents.this, color);
        }
    }

    /**
     * @see android.view.View#setLayerType()
     */
    public void setLayerType(int layerType, Paint paint) {
        mBvViewMethods.setLayerType(layerType, paint);
    }

    int getEffectiveBackgroundColor() {
        // Do not ask the WebContents for the background color, as it will always
        // report white prior to initial navigation or post destruction, whereas we want
        // to use the client supplied base value in those cases.
        if (isDestroyed(NO_WARN) || !mContentsClient.isCachedRendererBackgroundColorValid()) {
            return mBaseBackgroundColor;
        }
        return mContentsClient.getCachedRendererBackgroundColor();
    }

    public boolean isMultiTouchZoomSupported() {
        return mSettings.supportsMultiTouchZoom();
    }

    // jiang
    // public View getZoomControlsForTest() {
    // return mZoomControls.getZoomControlsViewForTest();
    // }

    /**
     * @see View#setOverScrollMode(int)
     */
    public void setOverScrollMode(int mode) {
        if (mode != View.OVER_SCROLL_NEVER) {
            mOverScrollGlow = new OverScrollGlow(mContext, mContainerView);
        } else {
            mOverScrollGlow = null;
        }
    }

    private boolean mOverlayHorizontalScrollbar = true;
    private boolean mOverlayVerticalScrollbar;

    /**
     * @see View#setScrollBarStyle(int)
     */
    public void setScrollBarStyle(int style) {
        if (style == View.SCROLLBARS_INSIDE_OVERLAY
                || style == View.SCROLLBARS_OUTSIDE_OVERLAY) {
            mOverlayHorizontalScrollbar = mOverlayVerticalScrollbar = true;
        } else {
            mOverlayHorizontalScrollbar = mOverlayVerticalScrollbar = false;
        }
    }

    /**
     * @see View#setHorizontalScrollbarOverlay(boolean)
     */
    public void setHorizontalScrollbarOverlay(boolean overlay) {
        if (TRACE)
            Log.i(TAG, "%s setHorizontalScrollbarOverlay=%s", this, overlay);
        mOverlayHorizontalScrollbar = overlay;
    }

    /**
     * @see View#setVerticalScrollbarOverlay(boolean)
     */
    public void setVerticalScrollbarOverlay(boolean overlay) {
        if (TRACE)
            Log.i(TAG, "%s setVerticalScrollbarOverlay=%s", this, overlay);
        mOverlayVerticalScrollbar = overlay;
    }

    /**
     * Called by the embedder when the scroll offset of the containing view has
     * changed.
     *
     * @see View#onScrollChanged(int, int)
     */
    public void onContainerViewScrollChanged(int l, int t, int oldl, int oldt) {
        mBvViewMethods.onContainerViewScrollChanged(l, t, oldl, oldt);
    }

    /**
     * Called by the embedder when the containing view is to be scrolled or
     * overscrolled.
     * @see View#onOverScrolled(int, int, int, int)
     */
    public void onContainerViewOverScrolled(int scrollX, int scrollY, boolean clampedX, boolean clampedY) {
        mBvViewMethods.onContainerViewOverScrolled(scrollX, scrollY, clampedX, clampedY);
    }

    /**
     * @see android.webkit.WebView#requestChildRectangleOnScreen(View, Rect, boolean)
     */
    public boolean requestChildRectangleOnScreen(View child, Rect rect, boolean immediate) {
        if (isDestroyed(WARN))
            return false;
        return mScrollOffsetManager.requestChildRectangleOnScreen(child.getLeft() - child.getScrollX(),
                child.getTop() - child.getScrollY(), rect, immediate);
    }

    /**
     * @see View#computeHorizontalScrollRange()
     */
    public int computeHorizontalScrollRange() {
        return mBvViewMethods.computeHorizontalScrollRange();
    }

    /**
     * @see View#computeHorizontalScrollOffset()
     */
    public int computeHorizontalScrollOffset() {
        return mBvViewMethods.computeHorizontalScrollOffset();
    }

    /**
     * @see View#computeVerticalScrollRange()
     */
    public int computeVerticalScrollRange() {
        return mBvViewMethods.computeVerticalScrollRange();
    }

    /**
     * @see View#computeVerticalScrollOffset()
     */
    public int computeVerticalScrollOffset() {
        return mBvViewMethods.computeVerticalScrollOffset();
    }

    /**
     * @see View#computeVerticalScrollExtent()
     */
    public int computeVerticalScrollExtent() {
        return mBvViewMethods.computeVerticalScrollExtent();
    }

    /**
     * @see View.computeScroll()
     */
    public void computeScroll() {
        mBvViewMethods.computeScroll();
    }

    /**
     * @see View#onCheckIsTextEditor()
     */
    public boolean onCheckIsTextEditor() {
        return mBvViewMethods.onCheckIsTextEditor();
    }

    /**
     * @see android.webkit.WebView#stopLoading()
     */
    public void stopLoading() {
        if (TRACE)
            Log.i(TAG, "%s stopLoading", this);
        if (!isDestroyed(WARN))
            mWebContents.stop();
    }

    public void reload() {
        if (TRACE)
            Log.i(TAG, "%s reload", this);
        if (!isDestroyed(WARN))
            mNavigationController.reload(true);
    }

    public boolean canGoBack() {
        return isDestroyed(WARN) ? false : mNavigationController.canGoBack();
    }

    public void goBack() {
        if (TRACE)
            Log.i(TAG, "%s goBack", this);
        if (!isDestroyed(WARN))
            mNavigationController.goBack();
    }

    public boolean canGoForward() {
        return isDestroyed(WARN) ? false : mNavigationController.canGoForward();
    }

    public void goForward() {
        if (TRACE)
            Log.i(TAG, "%s goForward", this);
        if (!isDestroyed(WARN))
            mNavigationController.goForward();
    }

    public boolean canGoBackOrForward(int steps) {
        return isDestroyed(WARN) ? false : mNavigationController.canGoToOffset(steps);
    }

    public void goBackOrForward(int steps) {
        if (TRACE)
            Log.i(TAG, "%s goBackOrForwad=%d", this, steps);
        if (!isDestroyed(WARN))
            mNavigationController.goToOffset(steps);
    }

    public void pauseTimers() {
        if (TRACE)
            Log.i(TAG, "%s pauseTimers", this);
        if (!isDestroyed(WARN)) {
            BvBrowserContext.getDefault().pauseTimers();
        }
    }

    public void resumeTimers() {
        if (TRACE)
            Log.i(TAG, "%s resumeTimers", this);
        if (!isDestroyed(WARN)) {
            BvBrowserContext.getDefault().resumeTimers();
        }
    }

    public void onPause() {
        if (TRACE)
            Log.i(TAG, "%s onPause", this);
        if (mIsPaused || isDestroyed(NO_WARN))
            return;
        mIsPaused = true;
        BvContentsJni.get().setIsPaused(mNativeBvContents, this, mIsPaused);
        updateWebContentsVisibility();
    }

    public void onResume() {
        if (TRACE)
            Log.i(TAG, "%s onResume", this);
        if (!mIsPaused || isDestroyed(NO_WARN))
            return;
        mIsPaused = false;
        BvContentsJni.get().setIsPaused(mNativeBvContents, this, mIsPaused);
        updateWebContentsVisibility();
    }

    /**
     * @see android.webkit.WebView#isPaused()
     */
    public boolean isPaused() {
        return isDestroyed(WARN) ? false : mIsPaused;
    }

    /**
     * @see android.webkit.WebView#onCreateInputConnection(EditorInfo)
     */
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        return mBvViewMethods.onCreateInputConnection(outAttrs);
    }

    /**
     * @see android.webkit.WebView#onDragEvent(DragEvent)
     */
    public boolean onDragEvent(DragEvent event) {
        return mBvViewMethods.onDragEvent(event);
    }

    /**
     * @see android.webkit.WebView#onKeyUp(int, KeyEvent)
     */
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        return mBvViewMethods.onKeyUp(keyCode, event);
    }

    /**
     * @see android.webkit.WebView#dispatchKeyEvent(KeyEvent)
     */
    public boolean dispatchKeyEvent(KeyEvent event) {
        return mBvViewMethods.dispatchKeyEvent(event);
    }

    /**
     * Clears the resource cache. Note that the cache is per-application, so this
     * will clear the cache for all WebViews used.
     *
     * @param includeDiskFiles if false, only the RAM cache is cleared
     */
    public void clearCache(boolean includeDiskFiles) {
        if (TRACE)
            Log.i(TAG, "%s clearCache", this);
        if (!isDestroyed(WARN)) {
            BvContentsJni.get().clearCache(mNativeBvContents, includeDiskFiles);
        }
    }

    public void documentHasImages(Message message) {
        if (!isDestroyed(WARN)) {
            BvContentsJni.get().documentHasImages(mNativeBvContents, message);
        }
    }

    public void saveWebArchive(final String basename, boolean autoname, final Callback<String> callback) {
        if (TRACE)
            Log.i(TAG, "%s saveWebArchive=%s", this, basename);

        if (!autoname) {
            saveWebArchiveInternal(basename, callback);
            return;
        }
        // If auto-generating the file name, handle the name generation on a background
        // thread
        // as it will require I/O access for checking whether previous files existed.
        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                return generateArchiveAutoNamePath(getOriginalUrl(), basename);
            }

            @Override
            protected void onPostExecute(String result) {
                saveWebArchiveInternal(result, callback);
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public String getOriginalUrl() {
        if (isDestroyed(WARN))
            return null;
        NavigationHistory history = mNavigationController.getNavigationHistory();
        int currentIndex = history.getCurrentEntryIndex();
        if (currentIndex >= 0 && currentIndex < history.getEntryCount()) {
            return history.getEntryAtIndex(currentIndex).getOriginalUrl().getSpec();
        }
        return null;
    }

    /**
     * @see NavigationController#getNavigationHistory()
     */
    public NavigationHistory getNavigationHistory() {
        return isDestroyed(WARN) ? null : mNavigationController.getNavigationHistory();
    }

    public String getTitle() {
        return isDestroyed(WARN) ? null : mWebContents.getTitle();
    }

    public void clearHistory() {
        if (TRACE)
            Log.i(TAG, "%s clearHistory", this);
        if (!isDestroyed(WARN))
            mNavigationController.clearHistory();
    }

    public SslCertificate getCertificate() {
        return isDestroyed(WARN) ? null
                : SslUtil.getCertificateFromDerBytes(
                        BvContentsJni.get().getCertificate(mNativeBvContents, BvContents.this));
    }

    public void clearSslPreferences() {
        if (TRACE)
            Log.i(TAG, "%s clearSslPreferences", this);
        if (!isDestroyed(WARN))
            mNavigationController.clearSslPreferences();
    }

    /**
     * Method to return all hit test values relevant to public WebView API. Note
     * that this expose more data than needed for WebView.getHitTestResult. Unsafely
     * returning reference to mutable internal object to avoid excessive garbage
     * allocation on repeated calls.
     */
    public HitTestData getLastHitTestResult() {
        if (TRACE)
            Log.i(TAG, "%s getLastHitTestResult", this);
        if (isDestroyed(WARN))
            return null;
        BvContentsJni.get().updateLastHitTestData(mNativeBvContents);
        return mPossiblyStaleHitTestData;
    }

    /**
     * @see android.webkit.WebView#requestFocusNodeHref()
     */
    public void requestFocusNodeHref(Message msg) {
        if (TRACE)
            Log.i(TAG, "%s requestFocusNodeHref", this);
        if (msg == null || isDestroyed(WARN))
            return;

        BvContentsJni.get().updateLastHitTestData(mNativeBvContents);
        Bundle data = msg.getData();

        // In order to maintain compatibility with the old WebView's implementation,
        // the absolute (full) url is passed in the |url| field, not only the href
        // attribute.
        data.putString("url", mPossiblyStaleHitTestData.href);
        data.putString("title", mPossiblyStaleHitTestData.anchorText);
        data.putString("src", mPossiblyStaleHitTestData.imgSrc);
        msg.setData(data);
        msg.sendToTarget();
    }

    /**
     * @see android.webkit.WebView#requestImageRef()
     */
    public void requestImageRef(Message msg) {
        if (TRACE)
            Log.i(TAG, "%s requestImageRef", this);
        if (msg == null || isDestroyed(WARN))
            return;

        BvContentsJni.get().updateLastHitTestData(mNativeBvContents);
        Bundle data = msg.getData();
        data.putString("url", mPossiblyStaleHitTestData.imgSrc);
        msg.setData(data);
        msg.sendToTarget();
    }

    private float getDeviceScaleFactor() {
        return mWindowAndroid.getWindowAndroid().getDisplay().getDipScale();
    }

    public ScriptReference addDocumentStartJavaScript(@NonNull String script, @NonNull String[] allowedOriginRules) {
        if (script == null) {
            throw new IllegalArgumentException("script shouldn't be null.");
        }

        for (int i = 0; i < allowedOriginRules.length; ++i) {
            if (TextUtils.isEmpty(allowedOriginRules[i])) {
                throw new IllegalArgumentException(
                        "allowedOriginRules[" + i + "] shouldn't be null or empty");
            }
        }

        return new ScriptReference(this,
                BvContentsJni.get().addDocumentStartJavaScript(
                        mNativeBvContents, this, script, allowedOriginRules));
    }

    void removeDocumentStartJavaScript(int scriptId) {
        BvContentsJni.get().removeDocumentStartJavaScript(mNativeBvContents, this, scriptId);
    }
    // private static void recordBaseUrl(@UrlScheme int value) {
    // RecordHistogram.recordEnumeratedHistogram(
    // DATA_BASE_URL_SCHEME_HISTOGRAM_NAME, value, UrlScheme.COUNT);
    // }

    // private static void recordLoadUrlScheme(@UrlScheme int value) {
    // RecordHistogram.recordEnumeratedHistogram(
    // LOAD_URL_SCHEME_HISTOGRAM_NAME, value, UrlScheme.COUNT);
    // }

    public void addWebMessageListener(@NonNull String jsObjectName,
            @NonNull String[] allowedOriginRules, @NonNull WebMessageListener listener) {
        if (TRACE)
            Log.i(TAG, "%s addWebMessageListener=%s", this, jsObjectName);
        if (listener == null) {
            throw new NullPointerException("listener shouldn't be null");
        }

        if (TextUtils.isEmpty(jsObjectName)) {
            throw new IllegalArgumentException("jsObjectName shouldn't be null or empty string");
        }

        for (int i = 0; i < allowedOriginRules.length; ++i) {
            if (TextUtils.isEmpty(allowedOriginRules[i])) {
                throw new IllegalArgumentException(
                        "allowedOriginRules[" + i + "] is null or empty");
            }
        }

        final String exceptionMessage =
                BvContentsJni.get().addWebMessageListener(mNativeBvContents, BvContents.this,
                        new WebMessageListenerHolder(listener), jsObjectName, allowedOriginRules);

        if (!TextUtils.isEmpty(exceptionMessage)) {
            throw new IllegalArgumentException(exceptionMessage);
        }
    }

    public void removeWebMessageListener(@NonNull String jsObjectName) {
        if (TRACE)
            Log.i(TAG, "%s removeWebMessageListener=%s", this, jsObjectName);
        BvContentsJni.get().removeWebMessageListener(
                mNativeBvContents, BvContents.this, jsObjectName);
    }

    @CalledByNative
    private void setAutofillClient(BvAutofillClient client) {
        mAutofillClient = client;
        client.init(mContext);
    }

    @CalledByNative
    private void onNativeDestroyed() {
        mWindowAndroid = null;
        mNativeBvContents = 0;
        mWebContents = null;
    }

    public static String sanitizeUrl(String url) {
        if (url == null)
            return null;
        if (url.startsWith("www.") || url.indexOf(":") == -1)
            url = "http://" + url;
        return url;
    }

    public void flingScroll(int velocityX, int velocityY) {
        if (TRACE) Log.i(TAG, "%s flingScroll", this);
        if (isDestroyed(WARN))
            return;
        mWebContents.getEventForwarder().startFling(
                SystemClock.uptimeMillis(), -velocityX, -velocityY, false, true);
    }

    public boolean pageUp(boolean top) {
        if (TRACE)
            Log.i(TAG, "%s pageUp", this);
        if (isDestroyed(WARN))
            return false;
        return mScrollOffsetManager.pageUp(top);
    }

    public boolean pageDown(boolean bottom) {
        if (TRACE)
            Log.i(TAG, "%s pageDown", this);
        if (isDestroyed(WARN))
            return false;
        return mScrollOffsetManager.pageDown(bottom);
    }

    public boolean canZoomIn() {
        if (isDestroyed(WARN))
            return false;
        final float zoomInExtent = mMaxPageScaleFactor - mPageScaleFactor;
        return zoomInExtent > ZOOM_CONTROLS_EPSILON;
    }

    public boolean canZoomOut() {
        if (isDestroyed(WARN))
            return false;
        final float zoomOutExtent = mPageScaleFactor - mMinPageScaleFactor;
        return zoomOutExtent > ZOOM_CONTROLS_EPSILON;
    }

    public boolean zoomIn() {
        if (!canZoomIn()) {
            return false;
        }
        zoomBy(1.25f);
        return true;
    }

    public boolean zoomOut() {
        if (!canZoomOut()) {
            return false;
        }
        zoomBy(0.8f);
        return true;
    }

    public void zoomBy(float delta) {
        if (isDestroyed(WARN))
            return;
        if (delta < 0.01f || delta > 100.0f) {
            throw new IllegalStateException("zoom delta value outside [0.01, 100] range.");
        }
        BvContentsJni.get().zoomBy(mNativeBvContents, this, delta);
    }

    public void evaluateJavaScript(String script, final Callback<String> callback) {
        if (TRACE) Log.i(TAG, "%s evaluateJavascript=%s", this, script);
        if (isDestroyed(WARN)) return;
        JavaScriptCallback jsCallback = null;
        if (callback != null) {
            jsCallback = jsonResult -> {
                PostTask.postTask(UiThreadTaskTraits.DEFAULT, () -> callback.onResult(jsonResult));
            };
        }
        mWebContents.evaluateJavaScript(script, jsCallback);
    }

    public void evaluateJavaScriptForTests(String script, final Callback<String> callback) {
        if (TRACE)
            Log.i(TAG, "%s evaluateJavascriptForTests=%s", this, script);
        if (isDestroyed(NO_WARN))
            return;
        JavaScriptCallback jsCallback = null;
        if (callback != null) {
            jsCallback = jsonResult -> callback.onResult(jsonResult);
        }

        mWebContents.evaluateJavaScriptForTests(script, jsCallback);
    }

    /**
     * Send a MessageEvent to main frame.
     *
     * @param message      The String message for the JavaScript MessageEvent.
     * @param targetOrigin The expected target frame's origin.
     * @param sentPorts    ports for the JavaScript MessageEvent.
     */
    public void postMessageToMainFrame(
            MessagePayload messagePayload, String targetOrigin, MessagePort[] sentPorts) {
        if (TRACE) Log.i(TAG, "%s postMessageToMainFrame", this);
        if (isDestroyed(WARN)) return;

        RenderFrameHost mainFrame = mWebContents.getMainFrame();
        // If the RenderFrameHost or the RenderFrame doesn't exist we couldn't post the message.
        if (mainFrame == null || !mainFrame.isRenderFrameLive()) return;

        mWebContents.postMessageToMainFrame(messagePayload, null, targetOrigin, sentPorts);
    }

    /**
     * Creates a message channel and returns the ports for each end of the channel.
     */
    public MessagePort[] createMessageChannel() {
        if (TRACE) Log.i(TAG, "%s createMessageChannel", this);
        if (isDestroyed(WARN)) return null;
        return MessagePort.createPair();
    }

    public boolean hasAccessedInitialDocument() {
        if (isDestroyed(NO_WARN))
            return false;
        return mWebContents.hasAccessedInitialDocument();
    }

    private WebContentsAccessibility getWebContentsAccessibility() {
        return WebContentsAccessibility.fromWebContents(mWebContents);
    }

    public void onProvideVirtualStructure(ViewStructure structure) {
        if (isDestroyed(WARN))
            return;
        if (!mWebContentsObserver.didEverCommitNavigation()) {
            structure.setChildCount(0);
            return;
        }
        getWebContentsAccessibility().onProvideVirtualStructure(structure, true);
    }

    public void onProvideAutoFillVirtualStructure(ViewStructure structure, int flags) {
        if (mAutofillProvider != null) {
            mAutofillProvider.onProvideAutoFillVirtualStructure(structure, flags);
        }
    }

    public void autofill(final SparseArray<AutofillValue> values) {
        if (mAutofillProvider != null) {
            mAutofillProvider.autofill(values);
        }
    }

    public boolean isSelectActionModeAllowed(int actionModeItem) {
        return (mSettings.getDisabledActionModeMenuItems() & actionModeItem) != actionModeItem;
    }

    void startActivityForResult(Intent intent, int requestCode) {
        // Even in fullscreen mode, startActivityForResult will still use the
        // initial internal access delegate because it has access to
        // the hidden API View#startActivityForResult.

        // mFullScreenTransitionsState.getInitialInternalAccessDelegate()
        // .super_startActivityForResult(intent, requestCode);
    }

    void startProcessTextIntent(Intent intent) {
        // on Android M, WebView is not able to replace the text with the processed
        // text.
        // So set the readonly flag for M.
        if (mContext == null)
            return;
        if (Build.VERSION.SDK_INT == Build.VERSION_CODES.M) {
            intent.putExtra(Intent.EXTRA_PROCESS_TEXT_READONLY, true);
        }

        if (ContextUtils.activityFromContext(mContext) == null) {
            mContext.startActivity(intent);
            return;
        }

        startActivityForResult(intent, PROCESS_TEXT_REQUEST_CODE);
    }

    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (isDestroyed(NO_WARN))
            return;
        if (requestCode == PROCESS_TEXT_REQUEST_CODE) {
            SelectionPopupController.fromWebContents(mWebContents)
                    .onReceivedProcessTextResult(resultCode, data);
        } else {
            Log.e(TAG, "Received activity result for an unknown request code %d", requestCode);
        }
    }

    /**
     * @see android.webkit.View#onTouchEvent()
     */
    public boolean onTouchEvent(MotionEvent event) {
        return mBvViewMethods.onTouchEvent(event);
    }

    /**
     * @see android.view.View#onHoverEvent()
     */
    public boolean onHoverEvent(MotionEvent event) {
        return mBvViewMethods.onHoverEvent(event);
    }

    /**
     * @see android.view.View#onGenericMotionEvent()
     */
    public boolean onGenericMotionEvent(MotionEvent event) {
        return isDestroyed(NO_WARN) ? false : mBvViewMethods.onGenericMotionEvent(event);
    }

    /**
     * @see android.view.View#onConfigurationChanged()
     */
    public void onConfigurationChanged(Configuration newConfig) {
        mBvViewMethods.onConfigurationChanged(newConfig);
    }

    /**
     * @see android.view.View#onAttachedToWindow()
     */
    public void onAttachedToWindow() {
        if (TRACE)
            Log.i(TAG, "%s onAttachedToWindow", this);
        mTemporarilyDetached = false;
        mBvViewMethods.onAttachedToWindow();
        mWindowAndroid.getWindowAndroid().getDisplay().addObserver(mDisplayObserver);
    }

    /**
     * @see android.view.View#onDetachedFromWindow()
     */
    @SuppressLint("MissingSuperCall")
    public void onDetachedFromWindow() {
        if (TRACE)
            Log.i(TAG, "%s onDetachedFromWindow", this);
        mWindowAndroid.getWindowAndroid().getDisplay().removeObserver(mDisplayObserver);
        mBvViewMethods.onDetachedFromWindow();
    }

    /**
     * @see android.view.View#onWindowFocusChanged()
     */
    public void onWindowFocusChanged(boolean hasWindowFocus) {
        mBvViewMethods.onWindowFocusChanged(hasWindowFocus);
    }

    /**
     * @see android.view.View#onFocusChanged()
     */
    public void onFocusChanged(boolean focused, int direction, Rect previouslyFocusedRect) {
        if (!mTemporarilyDetached) {
            mBvViewMethods.onFocusChanged(focused, direction, previouslyFocusedRect);
        }
    }

    /**
     * @see android.view.View#onSizeChanged()
     */
    public void onSizeChanged(int w, int h, int ow, int oh) {
        mBvViewMethods.onSizeChanged(w, h, ow, oh);
    }

    /**
     * @see android.view.View#onVisibilityChanged()
     */
    public void onVisibilityChanged(View changedView, int visibility) {
        mBvViewMethods.onVisibilityChanged(changedView, visibility);
    }

    /**
     * @see android.view.View#onWindowVisibilityChanged()
     */
    public void onWindowVisibilityChanged(int visibility) {
        mBvViewMethods.onWindowVisibilityChanged(visibility);
    }

    private void setViewVisibilityInternal(boolean visible) {
        mIsViewVisible = visible;
        if (!isDestroyed(NO_WARN)) {
            BvContentsJni.get().setViewVisibility(mNativeBvContents, BvContents.this, mIsViewVisible);
        }
        postUpdateWebContentsVisibility();
    }

    private void setWindowVisibilityInternal(boolean visible) {
        mIsWindowVisible = visible;
        if (!isDestroyed(NO_WARN)) {
            BvContentsJni.get().setWindowVisibility(mNativeBvContents, BvContents.this, mIsWindowVisible);
        }
        postUpdateWebContentsVisibility();
    }

    private void postUpdateWebContentsVisibility() {
        if (mIsUpdateVisibilityTaskPending)
            return;
        mIsUpdateVisibilityTaskPending = true;
        PostTask.postTask(UiThreadTaskTraits.DEFAULT, mUpdateVisibilityRunnable);
    }

    private void updateWebContentsVisibility() {
        mIsUpdateVisibilityTaskPending = false;
        if (isDestroyed(NO_WARN))
            return;
        boolean contentVisible = BvContentsJni.get().isVisible(mNativeBvContents, this);

        if (contentVisible && !mIsContentVisible) {
            mWebContents.onShow();
        } else {
            mWebContents.onHide();
        }
        mIsContentVisible = contentVisible;
        updateChildProcessImportance();
    }

    /**
     * Key for opaque state in bundle. Note this is only public for tests.
     */
    public static final String SAVE_RESTORE_STATE_KEY = "BISONVIEW_CHROMIUM_STATE";

    /**
     * Save the state of this BisonContents into provided Bundle.
     *
     * @return False if saving state failed.
     */
    public boolean saveState(Bundle outState) {
        if (TRACE)
            Log.i(TAG, "%s saveState", this);
        if (isDestroyed(WARN) || outState == null)
            return false;

        byte[] state = BvContentsJni.get().getOpaqueState(mNativeBvContents, BvContents.this);
        if (state == null)
            return false;

        outState.putByteArray(SAVE_RESTORE_STATE_KEY, state);
        return true;
    }

    /**
     * Restore the state of this BisonContents into provided Bundle.
     *
     * @param inState Must be a bundle returned by saveState.
     * @return False if restoring state failed.
     */
    public boolean restoreState(Bundle inState) {
        if (TRACE)
            Log.i(TAG, "%s restoreState", this);
        if (isDestroyed(WARN) || inState == null)
            return false;

        byte[] state = inState.getByteArray(SAVE_RESTORE_STATE_KEY);
        if (state == null)
            return false;

        boolean result = BvContentsJni.get().restoreFromOpaqueState(mNativeBvContents, BvContents.this, state);

        // The onUpdateTitle callback normally happens when a page is loaded,
        // but is optimized out in the restoreState case because the title is
        // already restored. See WebContentsImpl::UpdateTitleForEntry. So we
        // call the callback explicitly here.
        if (result)
            mContentsClient.onReceivedTitle(mWebContents.getTitle());

        return result;
    }

    public void addJavascriptInterface(Object object, String name) {
        if (TRACE)
            Log.i(TAG, "%s addJavascriptInterface=%s", this, name);
        if (isDestroyed(WARN))
            return;
        Class<? extends Annotation> requiredAnnotation = JavascriptInterface.class;
        getJavascriptInjector().addPossiblyUnsafeInterface(object, name, requiredAnnotation);
    }

    public void removeJavascriptInterface(String interfaceName) {
        if (TRACE)
            Log.i(TAG, "%s removeInterface=%s", this, interfaceName);
        if (isDestroyed(WARN))
            return;

        getJavascriptInjector().removeInterface(interfaceName);
    }

    /**
     * If native accessibility (not script injection) is enabled, and if this is
     * running on JellyBean or later, returns an AccessibilityNodeProvider that
     * implements native accessibility for this view. Returns null otherwise.
     *
     * @return The AccessibilityNodeProvider, if available, or null otherwise.
     */
    public AccessibilityNodeProvider getAccessibilityNodeProvider() {
        return mBvViewMethods.getAccessibilityNodeProvider();
    }

    public boolean supportsAccessibilityAction(int action) {
        return isDestroyed(WARN) ? false : getWebContentsAccessibility().supportsAction(action);
    }

    /**
     * @see android.webkit.WebView#performAccessibilityAction(int, Bundle)
     */
    public boolean performAccessibilityAction(int action, Bundle arguments) {
        return mBvViewMethods.performAccessibilityAction(action, arguments);
    }

    public void hideAutofillPopup() {
        if (TRACE) Log.i(TAG, "%s hideAutofillPopup", this);
        if (mAutofillClient != null) {
            mAutofillClient.hideAutofillPopup();
        }
        if (mAutofillProvider != null) {
            mAutofillProvider.hidePopup();
        }
    }

    public void setNetworkAvailable(boolean networkUp) {
        if (TRACE)
            Log.i(TAG, "%s setNetworkAvailable=%s", this, networkUp);
        if (!isDestroyed(WARN)) {
            // For backward compatibility when an app uses this API disable the
            // Network Information API to prevent inconsistencies,
            // see crbug.com/520088.
            NetworkChangeNotifier.setAutoDetectConnectivityState(false);
            BvContentsJni.get().setJsOnlineProperty(mNativeBvContents, BvContents.this, networkUp);
        }
    }

    /**
     * Inserts a {@link VisualStateCallback}.
     *
     * @param requestId an id that will be returned from the callback invocation to
     *                  allow callers to match requests with callbacks.
     * @param callback  the callback to be inserted
     * @throw IllegalStateException if this method is invoked after
     *        {@link #destroy()} has been called.
     */
    public void insertVisualStateCallback(long requestId, VisualStateCallback callback) {
        if (TRACE)
            Log.i(TAG, "%s insertVisualStateCallback", this);
        if (isDestroyed(NO_WARN))
            throw new IllegalStateException(
                    "insertVisualStateCallback cannot be called after the WebView has been destroyed");
        if (callback == null) {
            throw new IllegalArgumentException("VisualStateCallback shouldn't be null");
        }
        BvContentsJni.get().insertVisualStateCallback(mNativeBvContents, this, requestId, callback);
    }

    private void updateChildProcessImportance() {
        @ChildProcessImportance
        int effectiveImportance = ChildProcessImportance.IMPORTANT;
        if (mRendererPriorityWaivedWhenNotVisible && !mIsContentVisible) {
            effectiveImportance = ChildProcessImportance.NORMAL;
        } else {
            switch (mRendererPriority) {
                case RendererPriority.INITIAL:
                case RendererPriority.HIGH:
                    effectiveImportance = ChildProcessImportance.IMPORTANT;
                    break;
                case RendererPriority.LOW:
                    effectiveImportance = ChildProcessImportance.MODERATE;
                    break;
                case RendererPriority.WAIVED:
                    effectiveImportance = ChildProcessImportance.NORMAL;
                    break;
                default:
                    assert false;
            }
        }
        mWebContents.setImportance(effectiveImportance);
    }

    @RendererPriority
    public int getRendererRequestedPriority() {
        return mRendererPriority;
    }

    public boolean getRendererPriorityWaivedWhenNotVisible() {
        return mRendererPriorityWaivedWhenNotVisible;
    }

    public void setRendererPriorityPolicy(@RendererPriority int rendererRequestedPriority,
            boolean waivedWhenNotVisible) {
        mRendererPriority = rendererRequestedPriority;
        mRendererPriorityWaivedWhenNotVisible = waivedWhenNotVisible;
        updateChildProcessImportance();
    }

    public void setTextClassifier(TextClassifier textClassifier) {
        assert mWebContents != null;
        SelectionPopupController.fromWebContents(mWebContents).setTextClassifier(textClassifier);
    }

    public TextClassifier getTextClassifier() {
        assert mWebContents != null;
        return SelectionPopupController.fromWebContents(mWebContents).getTextClassifier();
    }

    public BvRenderProcess getRenderProcess() {
        if (isDestroyed(WARN)) {
            return null;
        }
        return BvContentsJni.get().getRenderProcess(mNativeBvContents, BvContents.this);
    }

    @CalledByNative
    private static void onDocumentHasImagesResponse(boolean result, Message message) {
        message.arg1 = result ? 1 : 0;
        message.sendToTarget();
    }

    @CalledByNative
    private void onReceivedTouchIconUrl(String url, boolean precomposed) {
        mContentsClient.onReceivedTouchIconUrl(url, precomposed);
    }

    @CalledByNative
    private void onReceivedIcon(Bitmap bitmap) {
        mContentsClient.onReceivedIcon(bitmap);
        mFavicon = bitmap;
    }

    /**
     * Callback for generateMHTML.
     */
    @CalledByNative
    private static void generateMHTMLCallback(String path, long size, Callback<String> callback) {
        if (callback == null)
            return;
        callback.onResult(size < 0 ? null : path);
    }

    @CalledByNative
    private void onReceivedHttpAuthRequest(BvHttpAuthHandler handler, String host, String realm) {
        mContentsClient.onReceivedHttpAuthRequest(handler, host, realm);
    }

    public BvGeolocationPermissions getGeolocationPermissions() {
        return mBrowserContext.getGeolocationPermissions();
    }

    public void invokeGeolocationCallback(boolean value, String requestingFrame) {
        if (isDestroyed(NO_WARN))
            return;
        BvContentsJni.get().invokeGeolocationCallback(mNativeBvContents, value, requestingFrame);
    }

    @CalledByNative
    private void onGeolocationPermissionsShowPrompt(String origin) {
        if (isDestroyed(NO_WARN))
            return;
        BvGeolocationPermissions permissions = mBrowserContext.getGeolocationPermissions();
        // Reject if geoloaction is disabled, or the origin has a retained deny
        if (!mSettings.getGeolocationEnabled()) {
            BvContentsJni.get().invokeGeolocationCallback(mNativeBvContents, false, origin);
            return;
        }
        // Allow if the origin has a retained allow
        if (permissions.hasOrigin(origin)) {
            BvContentsJni.get().invokeGeolocationCallback(mNativeBvContents, permissions.isOriginAllowed(origin),
                    origin);
            return;
        }
        mContentsClient.onGeolocationPermissionsShowPrompt(origin, new BvGeolocationCallback(origin, this));
    }

    @CalledByNative
    private void onGeolocationPermissionsHidePrompt() {
        mContentsClient.onGeolocationPermissionsHidePrompt();
    }

    @CalledByNative
    private void onPermissionRequest(BvPermissionRequest permissionRequest) {
        mContentsClient.onPermissionRequest(permissionRequest);
    }

    @CalledByNative
    private void onPermissionRequestCanceled(BvPermissionRequest permissionRequest) {
        mContentsClient.onPermissionRequestCanceled(permissionRequest);
    }

    @CalledByNative
    public void onFindResultReceived(int activeMatchOrdinal, int numberOfMatches, boolean isDoneCounting) {
        mContentsClient.onFindResultReceived(activeMatchOrdinal, numberOfMatches, isDoneCounting);
    }

    /**
     * Invokes the given {@link VisualStateCallback}.
     *
     * @param callback the callback to be invoked
     * @param requestId the id passed to
     *                  {@link BvContents#insertVisualStateCallback}
     * @param result    true if the callback should succeed and false otherwise
     */
    @CalledByNative
    public void invokeVisualStateCallback(final VisualStateCallback callback, final long requestId) {
        if (isDestroyed(NO_WARN))
            return;
        // Posting avoids invoking the callback inside invoking_composite_
        // (see synchronous_compositor_impl.cc and crbug/452530).
        PostTask.postTask(UiThreadTaskTraits.DEFAULT, () -> callback.onComplete(requestId));
    }

    // Called as a result of BvContentsJni.get().updateLastHitTestData.
    @CalledByNative
    private void updateHitTestData(
            int type, String extra, String href, String anchorText, String imgSrc) {
        mPossiblyStaleHitTestData.hitTestResultType = type;
        mPossiblyStaleHitTestData.hitTestResultExtraData = extra;
        mPossiblyStaleHitTestData.href = href;
        mPossiblyStaleHitTestData.anchorText = anchorText;
        mPossiblyStaleHitTestData.imgSrc = imgSrc;
    }

    private void postInvalidateOnAnimation() {
        // jiang
    }

    // jiang
    // @CalledByNative
    private int[] getLocationOnScreen() {
        int[] result = new int[2];
        if (mContainerView != null) {
            mContainerView.getLocationOnScreen(result);
        }
        return result;
    }

    @CalledByNative
    private void onWebLayoutPageScaleFactorChanged(float webLayoutPageScaleFactor) {
        // This change notification comes from the renderer thread, not from the cc/
        // impl thread.
        mLayoutSizer.onPageScaleChanged(webLayoutPageScaleFactor);
    }

    // @CalledByNative
    private void onWebLayoutContentsSizeChanged(int widthCss, int heightCss) {
        // This change notification comes from the renderer thread, not from the cc/
        // impl thread.
        mLayoutSizer.onContentSizeChanged(widthCss, heightCss);
    }

    private void saveWebArchiveInternal(String path, final Callback<String> callback) {
        if (path == null || isDestroyed(WARN)) {
            if (callback == null)
                return;

            PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> callback.onResult(null));
        } else {
            BvContentsJni.get().generateMHTML(mNativeBvContents, path, callback);
        }
    }

    /**
     * Try to generate a pathname for saving an MHTML archive. This roughly follows
     * WebView's autoname logic.
     */
    private static String generateArchiveAutoNamePath(String originalUrl, String baseName) {
        String name = null;
        if (originalUrl != null && !originalUrl.isEmpty()) {
            try {
                String path = new URL(originalUrl).getPath();
                int lastSlash = path.lastIndexOf('/');
                if (lastSlash > 0) {
                    name = path.substring(lastSlash + 1);
                } else {
                    name = path;
                }
            } catch (MalformedURLException e) {
                // If it fails parsing the URL, we'll just rely on the default name below.
            }
        }

        if (TextUtils.isEmpty(name))
            name = "index";

        String testName = baseName + name + WEB_ARCHIVE_EXTENSION;
        if (!new File(testName).exists())
            return testName;

        for (int i = 1; i < 100; i++) {
            testName = baseName + name + "-" + i + WEB_ARCHIVE_EXTENSION;
            if (!new File(testName).exists())
                return testName;
        }

        Log.e(TAG, "Unable to auto generate archive name for path: %s", baseName);
        return null;
    }

    @Override
    public void extractSmartClipData(int x, int y, int width, int height) {
        if (!isDestroyed(WARN)) {
            mWebContents.requestSmartClipExtract(x, y, width, height);
        }
    }

    @Override
    public void setSmartClipResultHandler(final Handler resultHandler) {
        if (isDestroyed(WARN))
            return;

        mWebContents.setSmartClipResultHandler(resultHandler);
    }

    protected void insertVisualStateCallbackIfNotDestroyed(long requestId, VisualStateCallback callback) {
        if (TRACE)
            Log.i(TAG, "%s insertVisualStateCallbackIfNotDestroyed", this);
        if (isDestroyed(NO_WARN))
            return;
        BvContentsJni.get().insertVisualStateCallback(mNativeBvContents, BvContents.this, requestId, callback);
    }

    public static boolean isDpadEvent(KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_DPAD_CENTER:
                case KeyEvent.KEYCODE_DPAD_DOWN:
                case KeyEvent.KEYCODE_DPAD_UP:
                case KeyEvent.KEYCODE_DPAD_LEFT:
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    return true;
            }
        }
        return false;
    }

    // Return true if the GeolocationPermissionAPI should be used.

    private class BvViewMethodsImpl implements BvViewMethods {
        private int mLayerType = View.LAYER_TYPE_NONE;
        // private ComponentCallbacks2 mComponentCallbacks;

        // Only valid within software onDraw().
        private final Rect mClipBoundsTemporary = new Rect();

        @Override
        public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            mLayoutSizer.onMeasure(widthMeasureSpec, heightMeasureSpec);
        }

        @Override
        public void requestFocus() {
            if (isDestroyed(NO_WARN) || mContainerView == null) return;
            if (!mContainerView.isInTouchMode() && mSettings.shouldFocusFirstNode()) {
                BvContentsJni.get().focusFirstNode(mNativeBvContents, BvContents.this);
            }
        }

        @Override
        public void setLayerType(int layerType, Paint paint) {
            mLayerType = layerType;
            updateHardwareAcceleratedFeaturesToggle();
        }

        private void updateHardwareAcceleratedFeaturesToggle() {
            if (mContainerView == null) return;
            mSettings.setEnableSupportedHardwareAcceleratedFeatures(
                    mIsAttachedToWindow && mContainerView.isHardwareAccelerated()
                            && (mLayerType == View.LAYER_TYPE_NONE || mLayerType == View.LAYER_TYPE_HARDWARE));
        }

        @Override
        public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
            return isDestroyed(NO_WARN)
                    ? null
                    : ImeAdapter.fromWebContents(mWebContents).onCreateInputConnection(outAttrs);
        }

        @Override
        public boolean onDragEvent(DragEvent event) {
            return isDestroyed(NO_WARN) || mContainerView == null
                    ? false
                    : mWebContents.getEventForwarder().onDragEvent(event, mContainerView);
        }

        @Override
        public boolean onKeyUp(int keyCode, KeyEvent event) {
            return isDestroyed(NO_WARN) ? false : mWebContents.getEventForwarder().onKeyUp(keyCode, event);
        }

        @Override
        public boolean dispatchKeyEvent(KeyEvent event) {
            if (isDestroyed(NO_WARN))
                return false;
            if (isDpadEvent(event)) {
                mSettings.setSpatialNavigationEnabled(true);
            }

            // Following check is dup'ed from |ContentUiEventHandler.dispatchKeyEvent| to
            // avoid
            // embedder-specific customization, which is necessary only for WebView.
            // if (GamepadList.dispatchKeyEvent(event))
            // return true;

            // // This check reflects Chrome's behavior and is a workaround for
            // // http://b/7697782.
            // if (mContentsClient.hasBisonViewClient() &&
            // mContentsClient.shouldOverrideKeyEvent(event)) {
            // return mInternalAccessAdapter.super_dispatchKeyEvent(event);
            // }
            return mWebContents.getEventForwarder().dispatchKeyEvent(event);
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if (isDestroyed(NO_WARN))
                return false;
            return mWebContents.getEventForwarder().onTouchEvent(event);
        }

        @Override
        public boolean onHoverEvent(MotionEvent event) {
            return isDestroyed(NO_WARN) ? false : mWebContents.getEventForwarder().onHoverEvent(event);
        }

        @Override
        public boolean onGenericMotionEvent(MotionEvent event) {
            return isDestroyed(NO_WARN) ? false : mWebContents.getEventForwarder().onGenericMotionEvent(event);
        }

        @Override
        public void onConfigurationChanged(Configuration newConfig) {
            if (!isDestroyed(NO_WARN)) {
                mViewEventSink.onConfigurationChanged(newConfig);
                mInternalAccessAdapter.super_onConfigurationChanged(newConfig);
            }
        }

        @Override
        public void onAttachedToWindow() {
            if (isDestroyed(NO_WARN))
                return;
            if (mIsAttachedToWindow) {
                Log.w(TAG, "onAttachedToWindow called when already attached. Ignoring");
                return;
            }
            mIsAttachedToWindow = true;

            mViewEventSink.onAttachedToWindow();
            // jiang
            // updateHardwareAcceleratedFeaturesToggle();
            postUpdateWebContentsVisibility();

            updateDefaultLocale();

            // if (mComponentCallbacks != null)
            // return;
            // mComponentCallbacks = new BvComponentCallbacks();
            // mContext.registerComponentCallbacks(mComponentCallbacks);
        }

        @Override
        public void onDetachedFromWindow() {
            if (isDestroyed(NO_WARN))
                return;
            if (!mIsAttachedToWindow) {
                Log.w(TAG, "onDetachedFromWindow called when already detached. Ignoring");
                return;
            }
            mIsAttachedToWindow = false;
            // hideAutofillPopup();
            // jiang
            // BvContentsJni.get().onDetachedFromWindow(mNativeBvContents,
            // BisonContents.this);

            mViewEventSink.onDetachedFromWindow();
            updateHardwareAcceleratedFeaturesToggle();
            postUpdateWebContentsVisibility();

            // if (mComponentCallbacks != null) {
            // mContext.unregisterComponentCallbacks(mComponentCallbacks);
            // mComponentCallbacks = null;
            // }

            // jiang
            // mScrollAccessibilityHelper.removePostedCallbacks();
            // mZoomControls.dismissZoomPicker();
        }

        @Override
        public void onWindowFocusChanged(boolean hasWindowFocus) {
            if (isDestroyed(NO_WARN))
                return;
            mWindowFocused = hasWindowFocus;
            mViewEventSink.onWindowFocusChanged(hasWindowFocus);
            Clipboard.getInstance().onWindowFocusChanged(hasWindowFocus);
        }

        @Override
        public void onFocusChanged(boolean focused, int direction, Rect previouslyFocusedRect) {
            if (isDestroyed(NO_WARN))
                return;
            mContainerViewFocused = focused;
            mViewEventSink.onViewFocusChanged(focused);
        }

        @Override
        public void onSizeChanged(int w, int h, int ow, int oh) {
            if (isDestroyed(NO_WARN))
                return;
            mScrollOffsetManager.setContainerViewSize(w, h);
            // The BvLayoutSizer needs to go first so that if we're in
            // fixedLayoutSize mode the update
            // to enter fixedLayoutSize mode is sent before the first resize
            // update.
            mLayoutSizer.onSizeChanged(w, h, ow, oh);
            BvContentsJni.get().onSizeChanged(mNativeBvContents, BvContents.this, w, h, ow, oh);
        }

        @Override
        public void onVisibilityChanged(View changedView, int visibility) {
            if (mContainerView == null)
                return;
            boolean viewVisible = mContainerView.getVisibility() == View.VISIBLE;
            if (mIsViewVisible == viewVisible)
                return;
            setViewVisibilityInternal(viewVisible);
        }

        @Override
        public void onWindowVisibilityChanged(int visibility) {
            boolean windowVisible = visibility == View.VISIBLE;
            if (mIsWindowVisible == windowVisible) return;
            setWindowVisibilityInternal(windowVisible);
        }

        @Override
        public void onContainerViewScrollChanged(int l, int t, int oldl, int oldt) {
            // A side-effect of View.onScrollChanged is that the scroll accessibility event
            // being
            // sent by the base class implementation. This is completely hidden from the
            // base
            // classes and cannot be prevented, which is why we need the code below.
            // jiang
            // mScrollAccessibilityHelper.removePostedViewScrolledAccessibilityEventCallback();
            mScrollOffsetManager.onContainerViewScrollChanged(l, t);
        }

        @Override
        public void onContainerViewOverScrolled(int scrollX, int scrollY, boolean clampedX, boolean clampedY) {
            if (mContainerView == null)
                return;
            int oldX = mContainerView.getScrollX();
            int oldY = mContainerView.getScrollY();

            mScrollOffsetManager.onContainerViewOverScrolled(scrollX, scrollY, clampedX, clampedY);

            if (mOverScrollGlow != null) {
                mOverScrollGlow.pullGlow(mContainerView.getScrollX(), mContainerView.getScrollY(), oldX, oldY,
                        mScrollOffsetManager.computeMaximumHorizontalScrollOffset(),
                        mScrollOffsetManager.computeMaximumVerticalScrollOffset());
            }
        }

        @Override
        public int computeHorizontalScrollRange() {
            return mScrollOffsetManager.computeHorizontalScrollRange();
        }

        @Override
        public int computeHorizontalScrollOffset() {
            return mScrollOffsetManager.computeHorizontalScrollOffset();
        }

        @Override
        public int computeVerticalScrollRange() {
            return mScrollOffsetManager.computeVerticalScrollRange();
        }

        @Override
        public int computeVerticalScrollOffset() {
            return mScrollOffsetManager.computeVerticalScrollOffset();
        }

        @Override
        public int computeVerticalScrollExtent() {
            return mScrollOffsetManager.computeVerticalScrollExtent();
        }

        @Override
        public void computeScroll() {
            if (isDestroyed(NO_WARN)) return;
            // BvContentsJni.get().onComputeScroll(mNativeBvContents, BvContents.this,
            // AnimationUtils.currentAnimationTimeMillis());
        }

        @Override
        public boolean onCheckIsTextEditor() {
            if (isDestroyed(NO_WARN))
                return false;
            ImeAdapter imeAdapter = ImeAdapter.fromWebContents(mWebContents);
            return imeAdapter != null ? imeAdapter.onCheckIsTextEditor() : false;
        }

        @Override
        public AccessibilityNodeProvider getAccessibilityNodeProvider() {
            if (isDestroyed(NO_WARN))
                return null;
            WebContentsAccessibility wcax = getWebContentsAccessibility();
            return wcax != null ? wcax.getAccessibilityNodeProvider() : null;
        }

        @Override
        public boolean performAccessibilityAction(final int action, final Bundle arguments) {
            if (isDestroyed(NO_WARN))
                return false;
            WebContentsAccessibility wcax = getWebContentsAccessibility();
            return wcax != null ? wcax.performAction(action, arguments) : false;
        }
    }

    @CalledByNative
    private boolean useLegacyGeolocationPermissionAPI() {
        // Always return true since we are not ready to swap the geolocation yet.
        // TODO: If we decide not to migrate the geolocation, there are some unreachable
        // code need to remove. http://crbug.com/396184.
        return true;
    }

    public void setWebContentsRenderView(int renderView) {

        mContentViewRenderView.requestMode(renderView, result -> {

        });
    }

    public static void logCommandLineForDebugging() {
        BvContentsJni.get().logCommandLineForDebugging();
    }

    @NativeMethods
    interface Natives {
        long init(BvContents caller, long nativeBvBrowserContext);

        void destroy(long nativeBvContents);

        void setShouldDownloadFavicons();
        void updateDefaultLocale(String locale, String localeList);

        void setJavaPeers(long nativeBvContents, BvWebContentsDelegate webContentsDelegate,
                BvContentsClientBridge bvContentsClientBridge, BvContentsIoThreadClient ioThreadClient,
                InterceptNavigationDelegate navigationInterceptionDelegate, AutofillProvider autofillProvider);

        WebContents getWebContents(long nativeBvContents);

        void documentHasImages(long nativeBvContents, Message message);

        void generateMHTML(long nativeBvContents, String path, Callback<String> callback);

        void zoomBy(long nativeBvContents, BvContents caller, float delta);

        void findAllAsync(long nativeBvContents, String searchString);

        void findNext(long nativeBvContents, boolean forward);

        void clearMatches(long nativeBvContents, BvContents caller);

        void clearCache(long nativeBvContents, boolean includeDiskFiles);

        byte[] getCertificate(long nativeBvContents, BvContents caller);

        void requestNewHitTestDataAt(long nativeBvContents, BvContents caller, float x, float y, float touchMajor);

        void setDipScale(long nativeBvContents, BvContents caller, float dipScale);

        byte[] getOpaqueState(long nativeBvContents, BvContents caller);

        boolean restoreFromOpaqueState(long nativeBvContents, BvContents caller, byte[] state);
        void updateLastHitTestData(long nativeBvContents);
        void onSizeChanged(long nativeBvContents, BvContents caller, int w, int h, int ow, int oh);

        void setViewVisibility(long nativeBvContents, BvContents caller, boolean visible);

        void setWindowVisibility(long nativeBvContents, BvContents caller, boolean visible);

        void setIsPaused(long nativeBvContents, BvContents caller, boolean paused);

        void onAttachedToWindow(long nativeBvContents, BvContents caller, int w, int h);

        void onDetachedFromWindow(long nativeBvContents, BvContents caller);

        boolean isVisible(long nativeBvContents, BvContents caller);

        void focusFirstNode(long nativeBvContents, BvContents caller);

        void setBackgroundColor(long nativeBvContents, BvContents caller, int color);

        void insertVisualStateCallback(long nativeBvContents, BvContents caller, long requestId,
                VisualStateCallback callback);

        void setExtraHeadersForUrl(
                long nativeBvContents, String url, String extraHeaders);

        void invokeGeolocationCallback(long nativeBvContents, boolean value, String requestingFrame);

        int getEffectivePriority(long nativeBvContents, BvContents caller);

        void setJsOnlineProperty(long nativeBvContents, BvContents caller, boolean networkUp);

        void createPdfExporter(long nativeBvContents, BvPdfExporter bvPdfExporter);

        void preauthorizePermission(long nativeBvContents, BvContents caller, String origin, long resources);

        void grantFileSchemeAccesstoChildProcess(long nativeBvContents);

        BvRenderProcess getRenderProcess(long nativeBvContents, BvContents caller);

        int addDocumentStartJavaScript(long nativeBvContents, BvContents caller, String script,
                String[] allowedOriginRules);

        void removeDocumentStartJavaScript(long nativeBvContents, BvContents caller, int scriptId);

        String addWebMessageListener(long nativeBvContents, BvContents caller,
                WebMessageListenerHolder listener, String jsObjectName, String[] allowedOrigins);

        void removeWebMessageListener(
                long nativeBvContents, BvContents caller, String jsObjectName);

        WebMessageListenerInfo[] getJsObjectsInfo(
                long nativeBvContents, BvContents caller, Class clazz);

        String getProductVersion();

        void logCommandLineForDebugging();
    }
}
