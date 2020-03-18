// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
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

import org.chromium.base.BuildInfo;
import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.LocaleUtils;
import org.chromium.base.Log;
import org.chromium.base.ObserverList;
import org.chromium.base.ThreadUtils;
import org.chromium.base.TraceEvent;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.CalledByNativeUnchecked;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.metrics.ScopedSysTraceEvent;
import org.chromium.base.task.AsyncTask;
import org.chromium.base.task.PostTask;
import org.chromium.components.autofill.AutofillProvider;
import org.chromium.components.content_capture.ContentCaptureConsumer;
import org.chromium.components.navigation_interception.InterceptNavigationDelegate;
import org.chromium.components.navigation_interception.NavigationParams;
import org.chromium.content_public.browser.ChildProcessImportance;
import org.chromium.content_public.browser.ContentViewStatics;
import org.chromium.content_public.browser.GestureListenerManager;
import org.chromium.content_public.browser.GestureStateListener;
import org.chromium.content_public.browser.ImeAdapter;
import org.chromium.content_public.browser.ImeEventObserver;
import org.chromium.content_public.browser.JavaScriptCallback;
import org.chromium.content_public.browser.JavascriptInjector;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.MessagePort;
import org.chromium.content_public.browser.NavigationController;
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
import org.chromium.content_public.common.UseZoomForDSFPolicy;
import org.chromium.device.gamepad.GamepadList;
import org.chromium.net.NetworkChangeNotifier;
import org.chromium.network.mojom.ReferrerPolicy;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.Clipboard;
import org.chromium.ui.base.PageTransition;
import org.chromium.ui.base.ViewAndroidDelegate;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.display.DisplayAndroid.DisplayAndroidObserver;

import java.io.File;
import java.lang.annotation.Annotation;
import java.lang.ref.WeakReference;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.WeakHashMap;
import java.util.concurrent.Callable;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import im.shimo.bison.gfx.BisonDrawFnImpl;
import im.shimo.bison.gfx.BisonFunctor;
import im.shimo.bison.gfx.BisonGLFunctor;
import im.shimo.bison.gfx.BisonPicture;
import im.shimo.bison.permission.BisonGeolocationCallback;
import im.shimo.bison.permission.BisonPermissionRequest;
import im.shimo.bison.renderer_priority.RendererPriority;

/**
 * Exposes the native BisonContents class, and together these classes wrap the WebContents
 * and Browser components that are required to implement Android WebView API. This is the
 * primary entry point for the WebViewProvider implementation; it holds a 1:1 object
 * relationship with application WebView instances.
 * (We define this class independent of the hidden WebViewProvider interfaces, to allow
 * continuous build &amp; test in the open source SDK-based tree).
 */
@JNINamespace("bison")
public class BisonContents implements SmartClipProvider {
    private static final String TAG = "BisonContents";
    private static final boolean TRACE = false;
    private static final int NO_WARN = 0;
    private static final int WARN = 1;
    private static final String PRODUCT_VERSION = BisonContentsStatics.getProductVersion();

    private static final String WEB_ARCHIVE_EXTENSION = ".mht";
    // The request code should be unique per WebView/BisonContents object.
    private static final int PROCESS_TEXT_REQUEST_CODE = 100;

    // Used to avoid enabling zooming in / out if resulting zooming will
    // produce little visible difference.
    private static final float ZOOM_CONTROLS_EPSILON = 0.007f;

    private static final double MIN_SCREEN_HEIGHT_PERCENTAGE_FOR_INTERSTITIAL = 0.7;

    private static final String SAMSUNG_WORKAROUND_BASE_URL = "email://";
    private static final int SAMSUNG_WORKAROUND_DELAY = 200;

    @VisibleForTesting
    public static final String DATA_BASE_URL_SCHEME_HISTOGRAM_NAME =
            "Android.WebView.LoadDataWithBaseUrl.BaseUrl";

    @VisibleForTesting
    public static final String LOAD_URL_SCHEME_HISTOGRAM_NAME = "Android.WebView.LoadUrl.UrlScheme";

    // Permit any number of slashes, since chromium seems to canonicalize bad values.
    private static final Pattern sFileAndroidAssetPattern =
            Pattern.compile("^file:/*android_(asset|res).*");

    // Matches a data URL that (may) have a valid fragment selector, pulling the fragment selector
    // out into a group. Such a URL must contain a single '#' character and everything after that
    // must be a valid DOM id.
    // DOM id grammar: https://www.w3.org/TR/1999/REC-html401-19991224/types.html#type-name
    private static final Pattern sDataURLWithSelectorPattern =
            Pattern.compile("^[^#]*(#[A-Za-z][A-Za-z0-9\\-_:.]*)$");

    private static class ForceAuxiliaryBitmapRendering {
        private static final boolean sResult = lazyCheck();
        private static boolean lazyCheck() {
            return !BisonContentsJni.get().hasRequiredHardwareExtensions();
        }
    }

    // Used to record the UMA histogram Android.WebView.LoadDataWithBaseUrl.HistoryUrl. Since these
    // values are persisted to logs, they should never be renumbered nor reused.
    @IntDef({HistoryUrl.EMPTY, HistoryUrl.BASEURL, HistoryUrl.DIFFERENT, HistoryUrl.COUNT})
    @interface HistoryUrl {
        int EMPTY = 0;
        int BASEURL = 1;
        int DIFFERENT = 2;
        int COUNT = 3;
    }

    // Used to record the UMA histogram Android.WebView.LoadDataWithBaseUrl.UrlScheme. Since these
    // values are persisted to logs, they should never be renumbered nor reused.
    @VisibleForTesting
    @IntDef({UrlScheme.EMPTY, UrlScheme.UNKNOWN_SCHEME, UrlScheme.HTTP_SCHEME,
            UrlScheme.HTTPS_SCHEME, UrlScheme.FILE_SCHEME, UrlScheme.FTP_SCHEME,
            UrlScheme.DATA_SCHEME, UrlScheme.JAVASCRIPT_SCHEME, UrlScheme.ABOUT_SCHEME,
            UrlScheme.CHROME_SCHEME, UrlScheme.BLOB_SCHEME, UrlScheme.CONTENT_SCHEME,
            UrlScheme.INTENT_SCHEME, UrlScheme.FILE_ANDROID_ASSET_SCHEME})
    public @interface UrlScheme {
        int EMPTY = 0;
        int UNKNOWN_SCHEME = 1;
        int HTTP_SCHEME = 2;
        int HTTPS_SCHEME = 3;
        int FILE_SCHEME = 4;
        int FTP_SCHEME = 5;
        int DATA_SCHEME = 6;
        int JAVASCRIPT_SCHEME = 7;
        int ABOUT_SCHEME = 8;
        int CHROME_SCHEME = 9;
        int BLOB_SCHEME = 10;
        int CONTENT_SCHEME = 11;
        int INTENT_SCHEME = 12;
        int FILE_ANDROID_ASSET_SCHEME = 13; // Covers android_asset and android_res URLs
        int COUNT = 14;
    }

    /**
     * WebKit hit test related data structure. These are used to implement
     * getHitTestResult, requestFocusNodeHref, requestImageRef methods in WebView.
     * All values should be updated together. The native counterpart is
     * BisonHitTestData.
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

    /**
     * Interface that consumers of {@link BisonContents} must implement to allow the proper
     * dispatching of view methods through the containing view.
     */
    public interface InternalAccessDelegate extends ViewEventSink.InternalAccessDelegate {
        /**
         * @see View#overScrollBy(int, int, int, int, int, int, int, int, boolean);
         */
        void overScrollBy(int deltaX, int deltaY,
                int scrollX, int scrollY,
                int scrollRangeX, int scrollRangeY,
                int maxOverScrollX, int maxOverScrollY,
                boolean isTouchEvent);

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
     * Factory interface used for constructing functors that the Android framework uses for
     * calling back into Chromium code to render the the contents of a Chromium frame into
     * an Android view.
     */
    public interface NativeDrawFunctorFactory {
        /**
         * Create a GL functor associated with native context |context|.
         */
        NativeDrawGLFunctor createGLFunctor(long context);

        /**
         * Used for draw_fn functor. Only one of these methods need to return non-null.
         * Prefer this over createGLFunctor.
         */
        BisonDrawFnImpl.DrawFnAccess getDrawFnAccess();

    }

    /**
     * Interface that consumers of {@link BisonContents} must implement to support
     * native GL rendering.
     */
    public interface NativeDrawGLFunctor {
        /**
         * Requests a callback on the native DrawGL method (see getBisonDrawGLFunction).
         *
         * If called from within onDraw, |canvas| should be non-null and must be hardware
         * accelerated. |releasedCallback| should be null if |canvas| is null, or if
         * supportsDrawGLFunctorReleasedCallback returns false.
         *
         * @return false indicates the GL draw request was not accepted, and the caller
         *         should fallback to the SW path.
         */
        boolean requestDrawGL(Canvas canvas, Runnable releasedCallback);

        /**
         * Requests a callback on the native DrawGL method (see getBisonDrawGLFunction).
         *
         * |containerView| must be hardware accelerated. If |waitForCompletion| is true, this method
         * will not return until functor has returned.
         */
        boolean requestInvokeGL(View containerView, boolean waitForCompletion);

        /**
         * Test whether the Android framework supports notifying when a functor is free
         * to be destroyed via the callback mechanism provided to the functor factory.
         *
         * @return true if destruction needs to wait on a framework callback, or false
         *         if it can occur immediately.
         */
        boolean supportsDrawGLFunctorReleasedCallback();

        /**
         * Detaches the GLFunctor from the view tree.
         */
        void detach(View containerView);

        /**
         * Destroy this functor instance and any native objects associated with it. No method is
         * called after destroy.
         */
        void destroy();
    }

    /**
     * Class to facilitate dependency injection. Subclasses by test code to provide mock versions of
     * certain BisonContents dependencies.
     */
    public static class DependencyFactory {
        public BisonLayoutSizer createLayoutSizer() {
            return new BisonLayoutSizer();
        }

        public BisonScrollOffsetManager createScrollOffsetManager(
                BisonScrollOffsetManager.Delegate delegate) {
            return new BisonScrollOffsetManager(delegate);
        }

        public AutofillProvider createAutofillProvider(Context context, ViewGroup containerView) {
            return null;
        }
    }

    /**
     * Visual state callback, see {@link #insertVisualStateCallback} for details.
     *
     */
    @VisibleForTesting
    public abstract static class VisualStateCallback {
        /**
         * @param requestId the id passed to {@link BisonContents#insertVisualStateCallback}
         * which can be used to match requests with the corresponding callbacks.
         */
        public abstract void onComplete(long requestId);
    }

    private long mNativeBisonContents;
    private BisonBrowserContext mBrowserContext;
    private ViewGroup mContainerView;
    private BisonFunctor mDrawFunctor;
    private final Context mContext;
    private final int mAppTargetSdkVersion;
    private BisonViewAndroidDelegate mViewAndroidDelegate;
    private WindowAndroidWrapper mWindowAndroid;
    private WebContents mWebContents;
    private ViewEventSink mViewEventSink;
    private WebContentsInternalsHolder mWebContentsInternalsHolder;
    private NavigationController mNavigationController;
    private final BisonContentsClient mContentsClient;
    private BisonWebContentsObserver mWebContentsObserver;
    private final BisonContentsClientBridge mContentsClientBridge;
    private final BisonWebContentsDelegateAdapter mWebContentsDelegate;
    private final BisonContentsBackgroundThreadClient mBackgroundThreadClient;
    private final BisonContentsIoThreadClient mIoThreadClient;
    private final InterceptNavigationDelegateImpl mInterceptNavigationDelegate;
    private InternalAccessDelegate mInternalAccessAdapter;
    private final NativeDrawFunctorFactory mNativeDrawFunctorFactory;
    private final BisonLayoutSizer mLayoutSizer;
    private final BisonZoomControls mZoomControls;
    private final BisonScrollOffsetManager mScrollOffsetManager;
    private OverScrollGlow mOverScrollGlow;
    private final DisplayAndroidObserver mDisplayObserver;
    // This can be accessed on any thread after construction. See BisonContentsIoThreadClient.
    private final BisonSettings mSettings;
    private final ScrollAccessibilityHelper mScrollAccessibilityHelper;

    private final ObserverList<PopupTouchHandleDrawable> mTouchHandleDrawables =
            new ObserverList<>();

    private boolean mIsPaused;
    private boolean mIsViewVisible;
    private boolean mIsWindowVisible;
    private boolean mIsAttachedToWindow;
    // Visiblity state of |mWebContents|.
    private boolean mIsContentVisible;
    private boolean mIsUpdateVisibilityTaskPending;
    private Runnable mUpdateVisibilityRunnable;

    private @RendererPriority int mRendererPriority;
    private boolean mRendererPriorityWaivedWhenNotVisible;

    private Bitmap mFavicon;
    private boolean mHasRequestedVisitedHistoryFromClient;
    // Whether this WebView is a popup.
    private boolean mIsPopupWindow;

    // The base background color, i.e. not accounting for any CSS body from the current page.
    private int mBaseBackgroundColor = Color.WHITE;

    // Must call BisonContentsJni.get().updateLastHitTestData first to update this before use.
    private final HitTestData mPossiblyStaleHitTestData = new HitTestData();

    private final DefaultVideoPosterRequestHandler mDefaultVideoPosterRequestHandler;

    // Bound method for suppling Picture instances to the BisonContentsClient. Will be null if the
    // picture listener API has not yet been enabled, or if it is using invalidation-only mode.
    private Callable<Picture> mPictureListenerContentProvider;

    private boolean mContainerViewFocused;
    private boolean mWindowFocused;

    // These come from the compositor and are updated synchronously (in contrast to the values in
    // RenderCoordinates, which are updated at end of every frame).
    private float mPageScaleFactor = 1.0f;
    private float mMinPageScaleFactor = 1.0f;
    private float mMaxPageScaleFactor = 1.0f;
    private float mContentWidthDip;
    private float mContentHeightDip;

    private BisonAutofillClient mBisonAutofillClient;

    private BisonPdfExporter mBisonPdfExporter;

    private BisonViewMethods mBisonViewMethods;
    private final FullScreenTransitionsState mFullScreenTransitionsState;

    // This flag indicates that ShouldOverrideUrlNavigation should be posted
    // through the resourcethrottle. This is only used for popup windows.
    private boolean mDeferredShouldOverrideUrlLoadingIsPendingForPopup;

    // This is a workaround for some qualcomm devices discarding buffer on
    // Activity restore.
    private boolean mInvalidateRootViewOnNextDraw;

    // The framework may temporarily detach our container view, for example during layout if
    // we are a child of a ListView. This may cause many toggles of View focus, which we suppress
    // when in this state.
    private boolean mTemporarilyDetached;

    // True when this BisonContents has been destroyed.
    // Do not use directly, call isDestroyed() instead.
    private boolean mIsDestroyed;

    private AutofillProvider mAutofillProvider;

    private static String sCurrentLocales = "";

    private Paint mPaintForNWorkaround;

    // A holder of objects passed from WebContents and should be owned by BisonContents that may
    // have direct or indirect reference back to WebView. They are used internally by
    // WebContents but all the references can create a new gc root that can keep WebView
    // instances from being freed when they are detached from view tree, hence lead to
    // memory leak. To avoid the issue, it is possible to use |WebContents.setInternalHolder|
    // to move the holder of those internal objects to BisonContents. Note that they are still
    // used by WebContents, and BisonContents doesn't have to know what's inside the holder.
    private WebContentsInternals mWebContentsInternals;

    private JavascriptInjector mJavascriptInjector;

    private ContentCaptureConsumer mContentCaptureConsumer;

    private static class WebContentsInternalsHolder implements WebContents.InternalsHolder {
        private final WeakReference<BisonContents> mBisonContentsRef;

        private WebContentsInternalsHolder(BisonContents awContents) {
            mBisonContentsRef = new WeakReference<>(awContents);
        }

        @Override
        public void set(WebContentsInternals internals) {
            BisonContents awContents = mBisonContentsRef.get();
            if (awContents == null) {
                throw new IllegalStateException("BisonContents should be available at this time");
            }
            awContents.mWebContentsInternals = internals;
        }

        @Override
        public WebContentsInternals get() {
            BisonContents awContents = mBisonContentsRef.get();
            return awContents == null ? null : awContents.mWebContentsInternals;
        }

        public boolean weakRefCleared() {
            return mBisonContentsRef.get() == null;
        }
    }

    private static final class BisonContentsDestroyRunnable implements Runnable {
        private final long mNativeBisonContents;
        // Hold onto a reference to the window (via its wrapper), so that it is not destroyed
        // until we are done here.
        private final WindowAndroidWrapper mWindowAndroid;

        private BisonContentsDestroyRunnable(
                long nativeBisonContents, WindowAndroidWrapper windowAndroid) {
            mNativeBisonContents = nativeBisonContents;
            mWindowAndroid = windowAndroid;
        }

        @Override
        public void run() {
            BisonContentsJni.get().destroy(mNativeBisonContents);
        }
    }

    /**
     * A class that stores the state needed to enter and exit fullscreen.
     */
    private static class FullScreenTransitionsState {
        private final ViewGroup mInitialContainerView;
        private final InternalAccessDelegate mInitialInternalAccessAdapter;
        private final BisonViewMethods mInitialBisonViewMethods;
        private FullScreenView mFullScreenView;
        /** Whether the initial container view was focused when we entered fullscreen */
        private boolean mWasInitialContainerViewFocused;
        private int mScrollX;
        private int mScrollY;

        private FullScreenTransitionsState(ViewGroup initialContainerView,
                InternalAccessDelegate initialInternalAccessAdapter,
                BisonViewMethods initialBisonViewMethods) {
            mInitialContainerView = initialContainerView;
            mInitialInternalAccessAdapter = initialInternalAccessAdapter;
            mInitialBisonViewMethods = initialBisonViewMethods;
        }

        private void enterFullScreen(FullScreenView fullScreenView,
                boolean wasInitialContainerViewFocused, int scrollX, int scrollY) {
            mFullScreenView = fullScreenView;
            mWasInitialContainerViewFocused = wasInitialContainerViewFocused;
            mScrollX = scrollX;
            mScrollY = scrollY;
        }

        private boolean wasInitialContainerViewFocused() {
            return mWasInitialContainerViewFocused;
        }

        private int getScrollX() {
            return mScrollX;
        }

        private int getScrollY() {
            return mScrollY;
        }

        private void exitFullScreen() {
            mFullScreenView = null;
        }

        private boolean isFullScreen() {
            return mFullScreenView != null;
        }

        private ViewGroup getInitialContainerView() {
            return mInitialContainerView;
        }

        private InternalAccessDelegate getInitialInternalAccessDelegate() {
            return mInitialInternalAccessAdapter;
        }

        private BisonViewMethods getInitialBisonViewMethods() {
            return mInitialBisonViewMethods;
        }

        private FullScreenView getFullScreenView() {
            return mFullScreenView;
        }
    }

    // Reference to the active mNativeBisonContents pointer while it is active use
    // (ie before it is destroyed).
    private CleanupReference mCleanupReference;

    //--------------------------------------------------------------------------------------------
    private class IoThreadClientImpl extends BisonContentsIoThreadClient {
        // All methods are called on the IO thread.

        @Override
        public int getCacheMode() {
            return mSettings.getCacheMode();
        }

        @Override
        public BisonContentsBackgroundThreadClient getBackgroundThreadClient() {
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

    private class BackgroundThreadClientImpl extends BisonContentsBackgroundThreadClient {
        // All methods are called on the background thread.

        @Override
        public BisonWebResourceResponse shouldInterceptRequest(
                BisonContentsClient.BisonWebResourceRequest request) {
            String url = request.url;
            BisonWebResourceResponse awWebResourceResponse;
            // Return the response directly if the url is default video poster url.
            awWebResourceResponse = mDefaultVideoPosterRequestHandler.shouldInterceptRequest(url);
            if (awWebResourceResponse != null) return awWebResourceResponse;

            awWebResourceResponse = mContentsClient.shouldInterceptRequest(request);

            if (awWebResourceResponse == null) {
                mContentsClient.getCallbackHelper().postOnLoadResource(url);
            }

            if (awWebResourceResponse != null) {
                String mimeType = awWebResourceResponse.getMimeType();
                if (mimeType == null) {
                    BisonHistogramRecorder.recordMimeType(
                            BisonHistogramRecorder.MimeType.NULL_FROM_SHOULD_INTERCEPT_REQUEST);
                } else {
                    BisonHistogramRecorder.recordMimeType(
                            BisonHistogramRecorder.MimeType.NONNULL_FROM_SHOULD_INTERCEPT_REQUEST);
                }
            }
            if (awWebResourceResponse != null && awWebResourceResponse.getData() == null) {
                // In this case the intercepted URLRequest job will simulate an empty response
                // which doesn't trigger the onReceivedError callback. For WebViewClassic
                // compatibility we synthesize that callback.  http://crbug.com/180950
                mContentsClient.getCallbackHelper().postOnReceivedError(
                        request,
                        /* error description filled in by the glue layer */
                        new BisonContentsClient.BisonWebResourceError());
            }
            return awWebResourceResponse;
        }
    }

    //--------------------------------------------------------------------------------------------
    // When the navigation is for a newly created WebView (i.e. a popup), intercept the navigation
    // here for implementing shouldOverrideUrlLoading. This is to send the shouldOverrideUrlLoading
    // callback to the correct WebViewClient that is associated with the WebView.
    // Otherwise, use this delegate only to post onPageStarted messages.
    //
    // We are not using WebContentsObserver.didStartLoading because of stale URLs, out of order
    // onPageStarted's and double onPageStarted's.
    //
    private class InterceptNavigationDelegateImpl implements InterceptNavigationDelegate {
        @Override
        public boolean shouldIgnoreNavigation(NavigationParams navigationParams) {
            // The shouldOverrideUrlLoading call might have resulted in posting messages to the
            // UI thread. Using sendMessage here (instead of calling onPageStarted directly)
            // will allow those to run in order.
            if (!BisonFeatureList.pageStartedOnCommitEnabled(navigationParams.isRendererInitiated)) {
                mContentsClient.getCallbackHelper().postOnPageStarted(navigationParams.url);
            }
            return false;
        }
    }

    //--------------------------------------------------------------------------------------------
    private class BisonLayoutSizerDelegate implements BisonLayoutSizer.Delegate {
        @Override
        public void requestLayout() {
            mContainerView.requestLayout();
        }

        @Override
        public void setMeasuredDimension(int measuredWidth, int measuredHeight) {
            mInternalAccessAdapter.setMeasuredDimension(measuredWidth, measuredHeight);
        }

        @Override
        public boolean isLayoutParamsHeightWrapContent() {
            return mContainerView.getLayoutParams() != null
                    && (mContainerView.getLayoutParams().height
                            == ViewGroup.LayoutParams.WRAP_CONTENT);
        }

        @Override
        public void setForceZeroLayoutHeight(boolean forceZeroHeight) {
            getSettings().setForceZeroLayoutHeight(forceZeroHeight);
        }
    }

    //--------------------------------------------------------------------------------------------
    private class BisonScrollOffsetManagerDelegate implements BisonScrollOffsetManager.Delegate {
        @Override
        public void overScrollContainerViewBy(int deltaX, int deltaY, int scrollX, int scrollY,
                int scrollRangeX, int scrollRangeY, boolean isTouchEvent) {
            mInternalAccessAdapter.overScrollBy(deltaX, deltaY, scrollX, scrollY,
                    scrollRangeX, scrollRangeY, 0, 0, isTouchEvent);
        }

        @Override
        public void scrollContainerViewTo(int x, int y) {
            mInternalAccessAdapter.super_scrollTo(x, y);
        }

        @Override
        public void scrollNativeTo(int x, int y) {
            if (!isDestroyed(NO_WARN)) {
                BisonContentsJni.get().scrollTo(mNativeBisonContents, BisonContents.this, x, y);
            }
        }

        @Override
        public void smoothScroll(int targetX, int targetY, long durationMs) {
            if (!isDestroyed(NO_WARN)) {
                BisonContentsJni.get().smoothScroll(
                        mNativeBisonContents, BisonContents.this, targetX, targetY, durationMs);
            }
        }

        @Override
        public int getContainerViewScrollX() {
            return mContainerView.getScrollX();
        }

        @Override
        public int getContainerViewScrollY() {
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

    //--------------------------------------------------------------------------------------------
    private class BisonGestureStateListener implements GestureStateListener {
        @Override
        public void onPinchStarted() {
            // While it's possible to re-layout the view during a pinch gesture, the effect is very
            // janky (especially that the page scale update notification comes from the renderer
            // main thread, not from the impl thread, so it's usually out of sync with what's on
            // screen). It's also quite expensive to do a re-layout, so we simply postpone
            // re-layout for the duration of the gesture. This is compatible with what
            // WebViewClassic does.
            mLayoutSizer.freezeLayoutRequests();
        }

        @Override
        public void onPinchEnded() {
            mLayoutSizer.unfreezeLayoutRequests();
        }

        @Override
        public void onScrollUpdateGestureConsumed() {
            mScrollAccessibilityHelper.postViewScrolledAccessibilityEventCallback();
            mZoomControls.invokeZoomPicker();
        }

        @Override
        public void onScrollStarted(int scrollOffsetY, int scrollExtentY) {
            mZoomControls.invokeZoomPicker();
        }

        @Override
        public void onScaleLimitsChanged(float minPageScaleFactor, float maxPageScaleFactor) {
            mZoomControls.updateZoomControls();
        }

        @Override
        public void onScrollOffsetOrExtentChanged(int scrollOffsetY, int scrollExtentY) {
            mZoomControls.updateZoomControls();
        }
    }

    //--------------------------------------------------------------------------------------------
    private class BisonComponentCallbacks implements ComponentCallbacks2 {
        @Override
        public void onTrimMemory(final int level) {
            boolean visibleRectEmpty = getGlobalVisibleRect().isEmpty();
            final boolean visible = mIsViewVisible && mIsWindowVisible && !visibleRectEmpty;
            ThreadUtils.runOnUiThreadBlocking(() -> {
                if (isDestroyed(NO_WARN)) return;
                if (level >= TRIM_MEMORY_MODERATE) {
                    if (mDrawFunctor != null) {
                        mDrawFunctor.trimMemory();
                    }
                }
                BisonContentsJni.get().trimMemory(mNativeBisonContents, BisonContents.this, level, visible);
            });
        }

        @Override
        public void onLowMemory() {}

        @Override
        public void onConfigurationChanged(Configuration configuration) {
            updateDefaultLocale();
        }
    };

    //--------------------------------------------------------------------------------------------
    private class BisonDisplayAndroidObserver implements DisplayAndroidObserver {
        @Override
        public void onRotationChanged(int rotation) {}

        @Override
        public void onDIPScaleChanged(float dipScale) {
            if (TRACE) Log.i(TAG, "%s onDIPScaleChanged dipScale=%f", this, dipScale);

            BisonContentsJni.get().setDipScale(mNativeBisonContents, BisonContents.this, dipScale);
            mLayoutSizer.setDIPScale(dipScale);
            mSettings.setDIPScale(dipScale);
        }
    };

    //--------------------------------------------------------------------------------------------
    /**
     * @param browserContext the browsing context to associate this view contents with.
     * @param containerView the view-hierarchy item this object will be bound to.
     * @param context the context to use, usually containerView.getContext().
     * @param internalAccessAdapter to access private methods on containerView.
     * @param nativeDrawFunctorFactory to access the functor provided by the WebView.
     * @param contentsClient will receive API callbacks from this WebView Contents.
     * @param bisonSettings BisonSettings instance used to configure the BisonContents.
     *
     * This constructor uses the default view sizing policy.
     */
    public BisonContents(BisonBrowserContext browserContext, ViewGroup containerView, Context context,
                         InternalAccessDelegate internalAccessAdapter,
                         NativeDrawFunctorFactory nativeDrawFunctorFactory, BisonContentsClient contentsClient,
                         BisonSettings bisonSettings) {
        this(browserContext, containerView, context, internalAccessAdapter,
                nativeDrawFunctorFactory, contentsClient, bisonSettings, new DependencyFactory());
    }

    /**
     * @param dependencyFactory an instance of the DependencyFactory used to provide instances of
     *                          classes that this class depends on.
     *
     * This version of the constructor is used in test code to inject test versions of the above
     * documented classes.
     */
    public BisonContents(BisonBrowserContext browserContext, ViewGroup containerView, Context context,
                         InternalAccessDelegate internalAccessAdapter,
                         NativeDrawFunctorFactory nativeDrawFunctorFactory, BisonContentsClient contentsClient,
                         BisonSettings settings, DependencyFactory dependencyFactory) {
        assert browserContext != null;
        try (ScopedSysTraceEvent e1 = ScopedSysTraceEvent.scoped("BisonContents.constructor")) {
            mRendererPriority = RendererPriority.HIGH;
            mSettings = settings;
            updateDefaultLocale();

            mBrowserContext = browserContext;

            // setWillNotDraw(false) is required since WebView draws its own contents using its
            // container view. If this is ever not the case we should remove this, as it removes
            // Android's gatherTransparentRegion optimization for the view.
            mContainerView = containerView;
            mContainerView.setWillNotDraw(false);

            mContext = context;
            mAutofillProvider = dependencyFactory.createAutofillProvider(context, mContainerView);
            mAppTargetSdkVersion = mContext.getApplicationInfo().targetSdkVersion;
            mInternalAccessAdapter = internalAccessAdapter;
            mNativeDrawFunctorFactory = nativeDrawFunctorFactory;
            mContentsClient = contentsClient;
            mContentsClient.getCallbackHelper().setCancelCallbackPoller(
                    () -> BisonContents.this.isDestroyed(NO_WARN));
            mBisonViewMethods = new BisonViewMethodsImpl();
            mFullScreenTransitionsState = new FullScreenTransitionsState(
                    mContainerView, mInternalAccessAdapter, mBisonViewMethods);
            mLayoutSizer = dependencyFactory.createLayoutSizer();
            mLayoutSizer.setDelegate(new BisonLayoutSizerDelegate());
            mWebContentsDelegate = new BisonWebContentsDelegateAdapter(
                    this, contentsClient, settings, mContext, mContainerView);
            mContentsClientBridge = new BisonContentsClientBridge(
                    mContext, contentsClient, BisonContentsStatics.getClientCertLookupTable());
            mZoomControls = new BisonZoomControls(this);
            mBackgroundThreadClient = new BackgroundThreadClientImpl();
            mIoThreadClient = new IoThreadClientImpl();
            mInterceptNavigationDelegate = new InterceptNavigationDelegateImpl();
            mDisplayObserver = new BisonDisplayAndroidObserver();
            mUpdateVisibilityRunnable = () -> updateWebContentsVisibility();

            BisonSettings.ZoomSupportChangeListener zoomListener =
                    (supportsDoubleTapZoom, supportsMultiTouchZoom) -> {
                if (isDestroyed(NO_WARN)) return;
                GestureListenerManager gestureManager =
                        GestureListenerManager.fromWebContents(mWebContents);
                gestureManager.updateDoubleTapSupport(supportsDoubleTapZoom);
                gestureManager.updateMultiTouchZoomSupport(supportsMultiTouchZoom);
            };
            mSettings.setZoomListener(zoomListener);
            mDefaultVideoPosterRequestHandler =
                    new DefaultVideoPosterRequestHandler(mContentsClient);
            mSettings.setDefaultVideoPosterURL(
                    mDefaultVideoPosterRequestHandler.getDefaultVideoPosterURL());
            mScrollOffsetManager = dependencyFactory.createScrollOffsetManager(
                    new BisonScrollOffsetManagerDelegate());
            mScrollAccessibilityHelper = new ScrollAccessibilityHelper(mContainerView);

            setOverScrollMode(mContainerView.getOverScrollMode());
            setScrollBarStyle(mInternalAccessAdapter.super_getScrollBarStyle());

            setNewBisonContents(BisonContentsJni.get().init(mBrowserContext.getNativePointer()));

            onContainerViewChanged();
        }
    }

    private void initWebContents(ViewAndroidDelegate viewDelegate,
            InternalAccessDelegate internalDispatcher, WebContents webContents,
            WindowAndroid windowAndroid, WebContentsInternalsHolder internalsHolder) {
        webContents.initialize(
                PRODUCT_VERSION, viewDelegate, internalDispatcher, windowAndroid, internalsHolder);
        mViewEventSink = ViewEventSink.from(mWebContents);
        mViewEventSink.setHideKeyboardOnBlur(false);
        SelectionPopupController controller = SelectionPopupController.fromWebContents(webContents);
        controller.setActionModeCallback(new BisonActionModeCallback(mContext, this, webContents));
        if (mAutofillProvider != null) {
            controller.setNonSelectionActionModeCallback(
                    new AutofillActionModeCallback(mContext, mAutofillProvider));
        }
        controller.setSelectionClient(SelectionClient.createSmartSelectionClient(webContents));

        // Listen for dpad events from IMEs (e.g. Samsung Cursor Control) so we know to enable
        // spatial navigation mode to allow these events to move focus out of the WebView.
        ImeAdapter.fromWebContents(webContents).addEventObserver(new ImeEventObserver() {
            @Override
            public void onBeforeSendKeyEvent(KeyEvent event) {
                if (BisonContents.isDpadEvent(event)) {
                    mSettings.setSpatialNavigationEnabled(true);
                }
            }
        });
    }

    private boolean isSamsungMailApp() {
        // There are 2 different Samsung mail apps exhibiting bugs related to
        // http://crbug.com/781535.
        String currentPackageName = mContext.getPackageName();
        return "com.android.email".equals(currentPackageName)
                || "com.samsung.android.email.composer".equals(currentPackageName);
    }

    boolean isFullScreen() {
        return mFullScreenTransitionsState.isFullScreen();
    }

    /**
     * Transitions this {@link BisonContents} to fullscreen mode and returns the
     * {@link View} where the contents will be drawn while in fullscreen, or null
     * if this BisonContents has already been destroyed.
     */
    View enterFullScreen() {
        assert !isFullScreen();
        if (isDestroyed(NO_WARN)) return null;

        // Detach to tear down the GL functor if this is still associated with the old
        // container view. It will be recreated during the next call to onDraw attached to
        // the new container view.
        onDetachedFromWindow();

        // In fullscreen mode FullScreenView owns the BisonViewMethodsImpl and BisonContents
        // a NullBisonViewMethods.
        FullScreenView fullScreenView = new FullScreenView(mContext, mBisonViewMethods, this,
                mContainerView.getWidth(), mContainerView.getHeight());
        fullScreenView.setFocusable(true);
        fullScreenView.setFocusableInTouchMode(true);
        boolean wasInitialContainerViewFocused = mContainerView.isFocused();
        if (wasInitialContainerViewFocused) {
            fullScreenView.requestFocus();
        }
        mFullScreenTransitionsState.enterFullScreen(fullScreenView, wasInitialContainerViewFocused,
                mScrollOffsetManager.getScrollX(), mScrollOffsetManager.getScrollY());
        mBisonViewMethods = new NullBisonViewMethods(this, mInternalAccessAdapter, mContainerView);

        // Associate this BisonContents with the FullScreenView.
        setInternalAccessAdapter(fullScreenView.getInternalAccessAdapter());
        setContainerView(fullScreenView);

        return fullScreenView;
    }

    /**
     * Called when the app has requested to exit fullscreen.
     */
    void requestExitFullscreen() {
        if (!isDestroyed(NO_WARN)) mWebContents.exitFullscreen();
    }

    /**
     * Returns this {@link BisonContents} to embedded mode, where the {@link BisonContents} are drawn
     * in the WebView.
     */
    void exitFullScreen() {
        if (!isFullScreen() || isDestroyed(NO_WARN)) {
            // exitFullScreen() can be called without a prior call to enterFullScreen() if a
            // "misbehave" app overrides onShowCustomView but does not add the custom view to
            // the window. Exiting avoids a crash.
            return;
        }

        // Detach to tear down the GL functor if this is still associated with the old
        // container view. It will be recreated during the next call to onDraw attached to
        // the new container view.
        // NOTE: we cannot use mBisonViewMethods here because its type is NullBisonViewMethods.
        BisonViewMethods awViewMethodsImpl = mFullScreenTransitionsState.getInitialBisonViewMethods();
        awViewMethodsImpl.onDetachedFromWindow();

        // Swap the view delegates. In embedded mode the FullScreenView owns a
        // NullBisonViewMethods and BisonContents the BisonViewMethodsImpl.
        FullScreenView fullscreenView = mFullScreenTransitionsState.getFullScreenView();
        fullscreenView.setBisonViewMethods(new NullBisonViewMethods(
                this, fullscreenView.getInternalAccessAdapter(), fullscreenView));
        mBisonViewMethods = awViewMethodsImpl;
        ViewGroup initialContainerView = mFullScreenTransitionsState.getInitialContainerView();

        // Re-associate this BisonContents with the WebView.
        setInternalAccessAdapter(mFullScreenTransitionsState.getInitialInternalAccessDelegate());
        setContainerView(initialContainerView);

        // Return focus to the WebView.
        if (mFullScreenTransitionsState.wasInitialContainerViewFocused()) {
            mContainerView.requestFocus();
        }

        if (!isDestroyed(NO_WARN)) {
            BisonContentsJni.get().restoreScrollAfterTransition(mNativeBisonContents, BisonContents.this,
                    mFullScreenTransitionsState.getScrollX(),
                    mFullScreenTransitionsState.getScrollY());
        }

        mFullScreenTransitionsState.exitFullScreen();
    }

    private void setInternalAccessAdapter(InternalAccessDelegate internalAccessAdapter) {
        mInternalAccessAdapter = internalAccessAdapter;
        mViewEventSink.setAccessDelegate(mInternalAccessAdapter);
    }

    private void setContainerView(ViewGroup newContainerView) {
        // setWillNotDraw(false) is required since WebView draws its own contents using its
        // container view. If this is ever not the case we should remove this, as it removes
        // Android's gatherTransparentRegion optimization for the view.
        mContainerView = newContainerView;
        mContainerView.setWillNotDraw(false);

        assert mDrawFunctor == null;

        mViewAndroidDelegate.setContainerView(mContainerView);
        if (mBisonPdfExporter != null) {
            mBisonPdfExporter.setContainerView(mContainerView);
        }
        mWebContentsDelegate.setContainerView(mContainerView);
        for (PopupTouchHandleDrawable drawable: mTouchHandleDrawables) {
            drawable.onContainerViewChanged(newContainerView);
        }
        onContainerViewChanged();
    }

    /**
     * Reconciles the state of this BisonContents object with the state of the new container view.
     */
    @SuppressLint("NewApi") // ViewGroup#isAttachedToWindow requires API level 19.
    private void onContainerViewChanged() {
        // NOTE: mBisonViewMethods is used by the old container view, the WebView, so it might refer
        // to a NullBisonViewMethods when in fullscreen. To ensure that the state is reconciled with
        // the new container view correctly, we bypass mBisonViewMethods and use the real
        // implementation directly.
        BisonViewMethods awViewMethodsImpl = mFullScreenTransitionsState.getInitialBisonViewMethods();
        awViewMethodsImpl.onVisibilityChanged(mContainerView, mContainerView.getVisibility());
        awViewMethodsImpl.onWindowVisibilityChanged(mContainerView.getWindowVisibility());

        boolean containerViewAttached = mContainerView.isAttachedToWindow();
        if (containerViewAttached && !mIsAttachedToWindow) {
            awViewMethodsImpl.onAttachedToWindow();
        } else if (!containerViewAttached && mIsAttachedToWindow) {
            awViewMethodsImpl.onDetachedFromWindow();
        }
        awViewMethodsImpl.onSizeChanged(
                mContainerView.getWidth(), mContainerView.getHeight(), 0, 0);
        awViewMethodsImpl.onWindowFocusChanged(mContainerView.hasWindowFocus());
        awViewMethodsImpl.onFocusChanged(mContainerView.hasFocus(), 0, null);
        mContainerView.requestLayout();
        if (mAutofillProvider != null) mAutofillProvider.onContainerViewChanged(mContainerView);
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
            try (ScopedSysTraceEvent e =
                            ScopedSysTraceEvent.scoped("WindowAndroidWrapper.constructor")) {
                mWindowAndroid = windowAndroid;
                mCleanupReference = new CleanupReference(this, new DestroyRunnable(windowAndroid));
            }
        }

        public WindowAndroid getWindowAndroid() {
            return mWindowAndroid;
        }
    }
    private static WeakHashMap<Context, WindowAndroidWrapper> sContextWindowMap;

    // getWindowAndroid is only called on UI thread, so there are no threading issues with lazy
    // initialization.
    private static WindowAndroidWrapper getWindowAndroid(Context context) {
        if (sContextWindowMap == null) sContextWindowMap = new WeakHashMap<>();
        WindowAndroidWrapper wrapper = sContextWindowMap.get(context);
        if (wrapper != null) return wrapper;

        try (ScopedSysTraceEvent e = ScopedSysTraceEvent.scoped("BisonContents.getWindowAndroid")) {
            boolean contextWrapsActivity = ContextUtils.activityFromContext(context) != null;
            if (contextWrapsActivity) {
                ActivityWindowAndroid activityWindow;
                try (ScopedSysTraceEvent e2 =
                                ScopedSysTraceEvent.scoped("BisonContents.createActivityWindow")) {
                    final boolean listenToActivityState = false;
                    activityWindow = new ActivityWindowAndroid(context, listenToActivityState);
                }
                wrapper = new WindowAndroidWrapper(activityWindow);
            } else {
                wrapper = new WindowAndroidWrapper(new WindowAndroid(context));
            }
            sContextWindowMap.put(context, wrapper);
        }
        return wrapper;
    }

    /**
     * Set current locales to native. Propagates this information to the Accept-Language header for
     * subsequent requests. Note that this will affect <b>all</b> BisonContents, not just this
     * instance, as all WebViews share the same NetworkContext/UrlRequestContextGetter.
     */
    @VisibleForTesting
    public void updateDefaultLocale() {
        String locales = LocaleUtils.getDefaultLocaleListString();
        if (!sCurrentLocales.equals(locales)) {
            sCurrentLocales = locales;

            // We cannot use the first language in sCurrentLocales for the UI language even on
            // Android N. LocaleUtils.getDefaultLocaleString() is capable for UI language but
            // it is not guaranteed to be listed at the first of sCurrentLocales. Therefore,
            // both values are passed to native.
            BisonContentsJni.get().updateDefaultLocale(
                    LocaleUtils.getDefaultLocaleString(), sCurrentLocales);
            mSettings.updateAcceptLanguages();
        }
    }

    private void setFunctor(BisonFunctor functor) {
        if (mDrawFunctor == functor) return;
        BisonFunctor oldFunctor = mDrawFunctor;
        mDrawFunctor = functor;
        updateNativeBisonGLFunctor();

        if (oldFunctor != null) oldFunctor.destroy();
    }

    private void updateNativeBisonGLFunctor() {
        BisonContentsJni.get().setCompositorFrameConsumer(mNativeBisonContents, BisonContents.this,
                mDrawFunctor != null ? mDrawFunctor.getNativeCompositorFrameConsumer() : 0);
    }

    /* Common initialization routine for adopting a native BisonContents instance into this
     * java instance.
     *
     * TAKE CARE! This method can get called multiple times per java instance. Code accordingly.
     * ^^^^^^^^^  See the native class declaration for more details on relative object lifetimes.
     */
    private void setNewBisonContents(long newBisonContentsPtr) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
            setNewBisonContentsPreO(newBisonContentsPtr);
        } else {
            // Move the TextClassifier to the new WebContents.
            TextClassifier textClassifier = mWebContents != null ? getTextClassifier() : null;
            setNewBisonContentsPreO(newBisonContentsPtr);
            if (textClassifier != null) setTextClassifier(textClassifier);
        }
        if (mContentCaptureConsumer != null) {
            mContentCaptureConsumer.onWebContentsChanged(mWebContents);
        }
    }

    // Helper for setNewBisonContents containing everything which applies to pre-O.
    private void setNewBisonContentsPreO(long newBisonContentsPtr) {
        if (mNativeBisonContents != 0) {
            destroyNatives();
            mWebContents = null;
            mWebContentsInternalsHolder = null;
            mWebContentsInternals = null;
            mNavigationController = null;
            mJavascriptInjector = null;
        }

        assert mNativeBisonContents == 0 && mCleanupReference == null && mWebContents == null;

        mNativeBisonContents = newBisonContentsPtr;
        updateNativeBisonGLFunctor();

        mWebContents = BisonContentsJni.get().getWebContents(mNativeBisonContents, BisonContents.this);
        mBrowserContext = BisonContentsJni.get().getBrowserContext(mNativeBisonContents, BisonContents.this);

        mWindowAndroid = getWindowAndroid(mContext);
        mViewAndroidDelegate =
                new BisonViewAndroidDelegate(mContainerView, mContentsClient, mScrollOffsetManager);
        mWebContentsInternalsHolder = new WebContentsInternalsHolder(this);
        initWebContents(mViewAndroidDelegate, mInternalAccessAdapter, mWebContents,
                mWindowAndroid.getWindowAndroid(), mWebContentsInternalsHolder);
        BisonContentsJni.get().setJavaPeers(mNativeBisonContents, BisonContents.this, this,
                mWebContentsDelegate, mContentsClientBridge, mIoThreadClient,
                mInterceptNavigationDelegate, mAutofillProvider);
        GestureListenerManager.fromWebContents(mWebContents)
                .addListener(new BisonGestureStateListener());

        mNavigationController = mWebContents.getNavigationController();
        installWebContentsObserver();
        mSettings.setWebContents(mWebContents);
        if (mAutofillProvider != null) mAutofillProvider.setWebContents(mWebContents);

        mDisplayObserver.onDIPScaleChanged(getDeviceScaleFactor());

        updateWebContentsVisibility();

        // The native side object has been bound to this java instance, so now is the time to
        // bind all the native->java relationships.
        mCleanupReference = new CleanupReference(
                this, new BisonContentsDestroyRunnable(mNativeBisonContents, mWindowAndroid));
    }

    private void installWebContentsObserver() {
        if (mWebContentsObserver != null) {
            mWebContentsObserver.destroy();
        }
        mWebContentsObserver = new BisonWebContentsObserver(mWebContents, this, mContentsClient);
    }

    /**
     * Called on the "source" BisonContents that is opening the popup window to
     * provide the BisonContents to host the pop up content.
     */
    public void supplyContentsForPopup(BisonContents newContents) {
        if (isDestroyed(WARN)) return;
        long popupNativeBisonContents =
                BisonContentsJni.get().releasePopupBisonContents(mNativeBisonContents, BisonContents.this);
        if (popupNativeBisonContents == 0) {
            Log.w(TAG, "Popup WebView bind failed: no pending content.");
            if (newContents != null) newContents.destroy();
            return;
        }
        if (newContents == null) {
            BisonContentsJni.get().destroy(popupNativeBisonContents);
            return;
        }

        newContents.receivePopupContents(popupNativeBisonContents);
    }

    // Recap: supplyContentsForPopup() is called on the parent window's content, this method is
    // called on the popup window's content.
    private void receivePopupContents(long popupNativeBisonContents) {
        if (isDestroyed(WARN)) return;
        mDeferredShouldOverrideUrlLoadingIsPendingForPopup = true;
        // Save existing view state.
        final boolean wasAttached = mIsAttachedToWindow;
        final boolean wasViewVisible = mIsViewVisible;
        final boolean wasWindowVisible = mIsWindowVisible;
        final boolean wasPaused = mIsPaused;
        final boolean wasFocused = mContainerViewFocused;
        final boolean wasWindowFocused = mWindowFocused;

        // Properly clean up existing mNativeBisonContents.
        if (wasFocused) onFocusChanged(false, 0, null);
        if (wasWindowFocused) onWindowFocusChanged(false);
        if (wasViewVisible) setViewVisibilityInternal(false);
        if (wasWindowVisible) setWindowVisibilityInternal(false);
        if (wasAttached) onDetachedFromWindow();
        if (!wasPaused) onPause();

        // Save injected JavaScript interfaces.
        Map<String, Pair<Object, Class>> javascriptInterfaces =
                new HashMap<String, Pair<Object, Class>>();
        if (mWebContents != null) {
            javascriptInterfaces.putAll(getJavascriptInjector().getInterfaces());
        }

        setNewBisonContents(popupNativeBisonContents);
        // We defer loading any URL on the popup until it has been properly intialized (through
        // setNewBisonContents). We resume the load here.
        BisonContentsJni.get().resumeLoadingCreatedPopupWebContents(
                mNativeBisonContents, BisonContents.this);

        // Finally refresh all view state for mNativeBisonContents.
        if (!wasPaused) onResume();
        if (wasAttached) {
            onAttachedToWindow();
            postInvalidateOnAnimation();
        }
        onSizeChanged(mContainerView.getWidth(), mContainerView.getHeight(), 0, 0);
        if (wasWindowVisible) setWindowVisibilityInternal(true);
        if (wasViewVisible) setViewVisibilityInternal(true);
        if (wasWindowFocused) onWindowFocusChanged(wasWindowFocused);
        if (wasFocused) onFocusChanged(true, 0, null);

        mIsPopupWindow = true;

        // Restore injected JavaScript interfaces.
        for (Map.Entry<String, Pair<Object, Class>> entry : javascriptInterfaces.entrySet()) {
            @SuppressWarnings("unchecked")
            Class<? extends Annotation> requiredAnnotation = entry.getValue().second;
            getJavascriptInjector().addPossiblyUnsafeInterface(
                    entry.getValue().first, entry.getKey(), requiredAnnotation);
        }
    }

    private JavascriptInjector getJavascriptInjector() {
        if (mJavascriptInjector == null) {
            mJavascriptInjector = JavascriptInjector.fromWebContents(mWebContents);
        }
        return mJavascriptInjector;
    }

    @CalledByNative
    private void onRendererResponsive(BisonRenderProcess renderProcess) {
        if (isDestroyed(NO_WARN)) return;
        mContentsClient.onRendererResponsive(renderProcess);
    }

    @CalledByNative
    private void onRendererUnresponsive(BisonRenderProcess renderProcess) {
        if (isDestroyed(NO_WARN)) return;
        mContentsClient.onRendererUnresponsive(renderProcess);
    }

    @VisibleForTesting
    @CalledByNativeUnchecked
    protected boolean onRenderProcessGone(int childProcessID, boolean crashed) {
        if (isDestroyed(NO_WARN)) return true;
        return mContentsClient.onRenderProcessGone(new BisonRenderProcessGoneDetail(crashed,
                BisonContentsJni.get().getEffectivePriority(mNativeBisonContents, BisonContents.this)));
    }

    @VisibleForTesting
    public @RendererPriority int getEffectivePriorityForTesting() {
        assert !isDestroyed(NO_WARN);
        return BisonContentsJni.get().getEffectivePriority(mNativeBisonContents, BisonContents.this);
    }

    /**
     * Destroys this object and deletes its native counterpart.
     */
    public void destroy() {
        if (TRACE) Log.i(TAG, "%s destroy", this);
        if (isDestroyed(NO_WARN)) return;

        if (mContentCaptureConsumer != null) {
            mContentCaptureConsumer.onWebContentsChanged(null);
            mContentCaptureConsumer = null;
        }

        // Remove pending messages
        mContentsClient.getCallbackHelper().removeCallbacksAndMessages();

        if (mIsAttachedToWindow) {
            Log.w(TAG, "WebView.destroy() called while WebView is still attached to window.");
            // Need to call detach to avoid leaks because the real detach later will be ignored.
            onDetachedFromWindow();
        }
        mIsDestroyed = true;
        PostTask.postTask(UiThreadTaskTraits.DEFAULT, () -> destroyNatives());
    }

    /**
     * Deletes the native counterpart of this object.
     */
    private void destroyNatives() {
        if (mCleanupReference != null) {
            assert mNativeBisonContents != 0;

            mWebContentsObserver.destroy();
            mWebContentsObserver = null;
            mNativeBisonContents = 0;
            mWebContents = null;
            mWebContentsInternals = null;
            mNavigationController = null;

            mCleanupReference.cleanupNow();
            mCleanupReference = null;
        }

        assert mWebContents == null;
        assert mNavigationController == null;
        assert mNativeBisonContents == 0;

        onDestroyed();
    }

    @VisibleForTesting
    protected void onDestroyed() {}

    /**
     * Returns whether this instance of WebView is flagged as destroyed.
     * If {@link WARN} is passed as a parameter, the method also issues a warning
     * log message and dumps stack, as embedders are advised not to call any
     * methods on destroyed WebViews.
     *
     * @param warnIfDestroyed use {@link WARN} if the check is done from a method
     * that is called via public WebView API, and {@link NO_WARN} otherwise.
     * @return whether this instance of WebView is flagged as destroyed.
     */
    private boolean isDestroyed(int warnIfDestroyed) {
        if (mIsDestroyed && warnIfDestroyed == WARN) {
            Log.w(TAG, "Application attempted to call on a destroyed WebView", new Throwable());
        }
        boolean destroyRunnableHasRun =
                mCleanupReference != null && mCleanupReference.hasCleanedUp();
        boolean weakRefsCleared =
                mWebContentsInternalsHolder != null && mWebContentsInternalsHolder.weakRefCleared();
        if (TRACE && destroyRunnableHasRun && !mIsDestroyed) {
            // Swallow the error. App developers are not going to do anything with an error msg.
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

    // Can be called from any thread.
    public BisonSettings getSettings() {
        return mSettings;
    }

    ViewGroup getContainerView() {
        return mContainerView;
    }

    public BisonPdfExporter getPdfExporter() {
        if (isDestroyed(WARN)) return null;
        if (mBisonPdfExporter == null) {
            mBisonPdfExporter = new BisonPdfExporter(mContainerView);
            BisonContentsJni.get().createPdfExporter(
                    mNativeBisonContents, BisonContents.this, mBisonPdfExporter);
        }
        return mBisonPdfExporter;
    }

    public static void setBisonDrawSWFunctionTable(long functionTablePointer) {
        BisonContentsJni.get().setBisonDrawSWFunctionTable(functionTablePointer);
    }

    public static void setBisonDrawGLFunctionTable(long functionTablePointer) {
        BisonContentsJni.get().setBisonDrawGLFunctionTable(functionTablePointer);
    }

    public static long getBisonDrawGLFunction() {
        return BisonGLFunctor.getBisonDrawGLFunction();
    }

    public static void setShouldDownloadFavicons() {
        BisonContentsJni.get().setShouldDownloadFavicons();
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

    /**
     * Intended for test code.
     * @return the number of native instances of this class.
     */
    @VisibleForTesting
    public static int getNativeInstanceCount() {
        return BisonContentsJni.get().getNativeInstanceCount();
    }

    // This is only to avoid heap allocations inside getGlobalVisibleRect. It should treated
    // as a local variable in the function and not used anywhere else.
    private static final Rect sLocalGlobalVisibleRect = new Rect();

    private Rect getGlobalVisibleRect() {
        if (!mContainerView.getGlobalVisibleRect(sLocalGlobalVisibleRect)) {
            sLocalGlobalVisibleRect.setEmpty();
        }
        return sLocalGlobalVisibleRect;
    }

    public void setContentCaptureConsumer(ContentCaptureConsumer consumer) {
        mContentCaptureConsumer = consumer;
    }

    //--------------------------------------------------------------------------------------------
    //  WebView[Provider] method implementations
    //--------------------------------------------------------------------------------------------

    public void onDraw(Canvas canvas) {
        try {
            TraceEvent.begin("BisonContents.onDraw");
            mBisonViewMethods.onDraw(canvas);
        } finally {
            TraceEvent.end("BisonContents.onDraw");
        }
    }

    public void setLayoutParams(final ViewGroup.LayoutParams layoutParams) {
        mLayoutSizer.onLayoutParamsChange();
    }

    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        mBisonViewMethods.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    public int getContentHeightCss() {
        if (isDestroyed(WARN)) return 0;
        return (int) Math.ceil(mContentHeightDip);
    }

    public int getContentWidthCss() {
        if (isDestroyed(WARN)) return 0;
        return (int) Math.ceil(mContentWidthDip);
    }

    public Picture capturePicture() {
        if (TRACE) Log.i(TAG, "%s capturePicture", this);
        if (isDestroyed(WARN)) return null;
        return new BisonPicture(BisonContentsJni.get().capturePicture(mNativeBisonContents, BisonContents.this,
                mScrollOffsetManager.computeHorizontalScrollRange(),
                mScrollOffsetManager.computeVerticalScrollRange()));
    }

    public void clearView() {
        if (TRACE) Log.i(TAG, "%s clearView", this);
        if (!isDestroyed(WARN)) BisonContentsJni.get().clearView(mNativeBisonContents, BisonContents.this);
    }

    /**
     * Enable the onNewPicture callback.
     * @param enabled Flag to enable the callback.
     * @param invalidationOnly Flag to call back only on invalidation without providing a picture.
     */
    public void enableOnNewPicture(boolean enabled, boolean invalidationOnly) {
        if (TRACE) Log.i(TAG, "%s enableOnNewPicture=%s", this, enabled);
        if (isDestroyed(WARN)) return;
        if (invalidationOnly) {
            mPictureListenerContentProvider = null;
        } else if (enabled && mPictureListenerContentProvider == null) {
            mPictureListenerContentProvider = () -> capturePicture();
        }
        BisonContentsJni.get().enableOnNewPicture(mNativeBisonContents, BisonContents.this, enabled);
    }

    public void findAllAsync(String searchString) {
        if (TRACE) Log.i(TAG, "%s findAllAsync", this);
        if (!isDestroyed(WARN)) {
            BisonContentsJni.get().findAllAsync(mNativeBisonContents, BisonContents.this, searchString);
        }
    }

    public void findNext(boolean forward) {
        if (TRACE) Log.i(TAG, "%s findNext", this);
        if (!isDestroyed(WARN)) {
            BisonContentsJni.get().findNext(mNativeBisonContents, BisonContents.this, forward);
        }
    }

    public void clearMatches() {
        if (TRACE) Log.i(TAG, "%s clearMatches", this);
        if (!isDestroyed(WARN)) {
            BisonContentsJni.get().clearMatches(mNativeBisonContents, BisonContents.this);
        }
    }

    /**
     * @return load progress of the WebContents.
     */
    public int getMostRecentProgress() {
        if (isDestroyed(WARN)) return 0;
        // WebContentsDelegateAndroid conveniently caches the most recent notified value for us.
        return mWebContentsDelegate.getMostRecentProgress();
    }

    public Bitmap getFavicon() {
        if (isDestroyed(WARN)) return null;
        return mFavicon;
    }

    private void requestVisitedHistoryFromClient() {
        Callback<String[]> callback = value -> {
            if (value != null) {
                // Replace null values with empty strings, because they can't be represented as
                // native strings.
                for (int i = 0; i < value.length; i++) {
                    if (value[i] == null) value[i] = "";
                }
            }

            PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> {
                if (!isDestroyed(NO_WARN)) {
                    BisonContentsJni.get().addVisitedLinks(mNativeBisonContents, BisonContents.this, value);
                }
            });
        };
        mContentsClient.getVisitedHistory(callback);
    }

    /**
     * WebView.loadUrl.
     */
    public void loadUrl(String url, Map<String, String> additionalHttpHeaders) {
        if (TRACE) Log.i(TAG, "%s loadUrl(extra headers)=%s", this, url);
        if (isDestroyed(WARN)) return;
        // Early out to match old WebView implementation
        if (url == null) {
            return;
        }
        // TODO: We may actually want to do some sanity checks here (like filter about://chrome).

        // For backwards compatibility, apps targeting less than K will have JS URLs evaluated
        // directly and any result of the evaluation will not replace the current page content.
        // Matching Chrome behavior more closely; apps targetting >= K that load a JS URL will
        // have the result of that URL replace the content of the current page.
        final String javaScriptScheme = "javascript:";
        if (mAppTargetSdkVersion < Build.VERSION_CODES.KITKAT && url.startsWith(javaScriptScheme)) {
            evaluateJavaScript(url.substring(javaScriptScheme.length()), null);
            return;
        }

        LoadUrlParams params = new LoadUrlParams(url, PageTransition.TYPED);
        if (additionalHttpHeaders != null) {
            params.setExtraHeaders(new HashMap<String, String>(additionalHttpHeaders));
        }

        loadUrl(params);
    }

    /**
     * WebView.loadUrl.
     */
    public void loadUrl(String url) {
        if (TRACE) Log.i(TAG, "%s loadUrl=%s", this, url);
        if (isDestroyed(WARN)) return;
        // Early out to match old WebView implementation
        if (url == null) {
            return;
        }
        loadUrl(url, null);
    }

    /**
     * WebView.postUrl.
     */
    public void postUrl(String url, byte[] postData) {
        if (TRACE) Log.i(TAG, "%s postUrl=%s", this, url);
        if (isDestroyed(WARN)) return;
        LoadUrlParams params = LoadUrlParams.createLoadHttpPostParams(url, postData);
        Map<String, String> headers = new HashMap<String, String>();
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

    private static void recordHistoryUrl(@HistoryUrl int value) {
        RecordHistogram.recordEnumeratedHistogram(
                "Android.WebView.LoadDataWithBaseUrl.HistoryUrl", value, HistoryUrl.COUNT);
    }

    private static void recordBaseUrl(@UrlScheme int value) {
        RecordHistogram.recordEnumeratedHistogram(
                DATA_BASE_URL_SCHEME_HISTOGRAM_NAME, value, UrlScheme.COUNT);
    }

    private static void recordLoadUrlScheme(@UrlScheme int value) {
        RecordHistogram.recordEnumeratedHistogram(
                LOAD_URL_SCHEME_HISTOGRAM_NAME, value, UrlScheme.COUNT);
    }

    /**
     * WebView.loadData.
     */
    public void loadData(String data, String mimeType, String encoding) {
        if (TRACE) Log.i(TAG, "%s loadData", this);
        if (isDestroyed(WARN)) return;
        if (data != null && data.contains("#")) {
            if (!BuildInfo.targetsAtLeastQ() && !isBase64Encoded(encoding)) {
                // As of Chromium M72, data URI parsing strictly enforces encoding of '#'. To
                // support WebView applications which were not expecting this change, we do it for
                // them.
                data = fixupOctothorpesInLoadDataContent(data);
            }
        }
        loadUrl(LoadUrlParams.createLoadDataParams(
                fixupData(data), fixupMimeType(mimeType), isBase64Encoded(encoding)));
    }

    /**
     * Helper method to fixup content passed to {@link #loadData} which may not have had '#'
     * characters encoded correctly. Historically Chromium did not strictly enforce the encoding of
     * '#' characters in Data URLs; they would be treated both as renderable content and as
     * potential URL fragments for DOM id matching. This behavior changed in Chromium M72 where
     * stricter parsing was enforced; the first '#' character now marks the end of the renderable
     * section and the start of the DOM fragment.
     *
     * @param data The content passed to {@link #loadData}, which may contain unencoded '#'s.
     * @return A version of the input with '#' characters correctly encoded, preserving any DOM id
     *         selector which may have been present in the original.
     */
    @VisibleForTesting
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

    private @UrlScheme int schemeForUrl(String url) {
        if (url == null || url.equals(ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL)) {
            return (UrlScheme.EMPTY);
        } else if (url.startsWith("http:")) {
            return (UrlScheme.HTTP_SCHEME);
        } else if (url.startsWith("https:")) {
            return (UrlScheme.HTTPS_SCHEME);
        } else if (sFileAndroidAssetPattern.matcher(url).matches()) {
            return (UrlScheme.FILE_ANDROID_ASSET_SCHEME);
        } else if (url.startsWith("file:")) {
            return (UrlScheme.FILE_SCHEME);
        } else if (url.startsWith("ftp:")) {
            return (UrlScheme.FTP_SCHEME);
        } else if (url.startsWith("data:")) {
            return (UrlScheme.DATA_SCHEME);
        } else if (url.startsWith("javascript:")) {
            return (UrlScheme.JAVASCRIPT_SCHEME);
        } else if (url.startsWith("about:")) {
            return (UrlScheme.ABOUT_SCHEME);
        } else if (url.startsWith("chrome:")) {
            return (UrlScheme.CHROME_SCHEME);
        } else if (url.startsWith("blob:")) {
            return (UrlScheme.BLOB_SCHEME);
        } else if (url.startsWith("content:")) {
            return (UrlScheme.CONTENT_SCHEME);
        } else if (url.startsWith("intent:")) {
            return (UrlScheme.INTENT_SCHEME);
        }
        return (UrlScheme.UNKNOWN_SCHEME);
    }

    /**
     * WebView.loadDataWithBaseURL.
     */
    public void loadDataWithBaseURL(
            String baseUrl, String data, String mimeType, String encoding, String historyUrl) {
        if (TRACE) Log.i(TAG, "%s loadDataWithBaseURL=%s", this, baseUrl);
        if (isDestroyed(WARN)) return;

        data = fixupData(data);
        mimeType = fixupMimeType(mimeType);
        LoadUrlParams loadUrlParams;
        baseUrl = fixupBase(baseUrl);
        historyUrl = fixupHistory(historyUrl);

        if (historyUrl.equals(ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL)) {
            recordHistoryUrl(HistoryUrl.EMPTY);
        } else if (historyUrl.equals(baseUrl)) {
            recordHistoryUrl(HistoryUrl.BASEURL);
        } else {
            recordHistoryUrl(HistoryUrl.DIFFERENT);
        }

        recordBaseUrl(schemeForUrl(baseUrl));

        if (baseUrl.startsWith("data:")) {
            // For backwards compatibility with WebViewClassic, we use the value of |encoding|
            // as the charset, as long as it's not "base64".
            boolean isBase64 = isBase64Encoded(encoding);
            loadUrlParams = LoadUrlParams.createLoadDataParamsWithBaseUrl(
                    data, mimeType, isBase64, baseUrl, historyUrl, isBase64 ? null : encoding);
        } else {
            // When loading data with a non-data: base URL, the classic WebView would effectively
            // "dump" that string of data into the WebView without going through regular URL
            // loading steps such as decoding URL-encoded entities. We achieve this same behavior by
            // base64 encoding the data that is passed here and then loading that as a data: URL.
            try {
                loadUrlParams = LoadUrlParams.createLoadDataParamsWithBaseUrl(
                        Base64.encodeToString(data.getBytes("utf-8"), Base64.DEFAULT), mimeType,
                        true, baseUrl, historyUrl, "utf-8");
            } catch (java.io.UnsupportedEncodingException e) {
                Log.wtf(TAG, "Unable to load data string %s", data, e);
                return;
            }
        }

        // This is a workaround for an issue with PlzNavigate and one of Samsung's OEM mail apps.
        // See http://crbug.com/781535.
        if (isSamsungMailApp() && SAMSUNG_WORKAROUND_BASE_URL.equals(loadUrlParams.getBaseUrl())) {
            PostTask.postDelayedTask(UiThreadTaskTraits.DEFAULT,
                    () -> loadUrl(loadUrlParams), SAMSUNG_WORKAROUND_DELAY);
            return;
        }
        loadUrl(loadUrlParams);
    }

    /**
     * Load url without fixing up the url string. Consumers of ContentView are responsible for
     * ensuring the URL passed in is properly formatted (i.e. the scheme has been added if left
     * off during user input).
     *
     * @param params Parameters for this load.
     */
    @VisibleForTesting
    public void loadUrl(LoadUrlParams params) {
        if (params.getBaseUrl() == null) {
            // Don't record the URL if this was loaded via loadDataWithBaseURL(). That API is
            // tracked separately under Android.WebView.LoadDataWithBaseUrl.BaseUrl.
            recordLoadUrlScheme(schemeForUrl(params.getUrl()));
        }

        if (params.getLoadUrlType() == LoadURLType.DATA && !params.isBaseUrlDataScheme()) {
            // This allows data URLs with a non-data base URL access to file:///android_asset/ and
            // file:///android_res/ URLs. If BisonSettings.getAllowFileAccess permits, it will also
            // allow access to file:// URLs (subject to OS level permission checks).
            params.setCanLoadLocalResources(true);
            BisonContentsJni.get().grantFileSchemeAccesstoChildProcess(
                    mNativeBisonContents, BisonContents.this);
        }

        // If we are reloading the same url, then set transition type as reload.
        if (params.getUrl() != null && params.getUrl().equals(mWebContents.getLastCommittedUrl())
                && params.getTransitionType() == PageTransition.TYPED) {
            params.setTransitionType(PageTransition.RELOAD);
        }
        params.setTransitionType(
                params.getTransitionType() | PageTransition.FROM_API);

        // For WebView, always use the user agent override, which is set
        // every time the user agent in BisonSettings is modified.
        params.setOverrideUserAgent(UserAgentOverrideOption.TRUE);


        // We don't pass extra headers to the content layer, as WebViewClassic
        // was adding them in a very narrow set of conditions. See http://crbug.com/306873
        // However, if the embedder is attempting to inject a Referer header for their
        // loadUrl call, then we set that separately and remove it from the extra headers map/
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

        BisonContentsJni.get().setExtraHeadersForUrl(mNativeBisonContents, BisonContents.this,
                params.getUrl(), params.getExtraHttpRequestHeadersString());
        params.setExtraHeaders(new HashMap<String, String>());

        mNavigationController.loadUrl(params);

        // The behavior of WebViewClassic uses the populateVisitedLinks callback in WebKit.
        // Chromium does not use this use code path and the best emulation of this behavior to call
        // request visited links once on the first URL load of the WebView.
        if (!mHasRequestedVisitedHistoryFromClient) {
            mHasRequestedVisitedHistoryFromClient = true;
            requestVisitedHistoryFromClient();
        }
    }

    /**
     * Get the URL of the current page. This is the visible URL of the {@link WebContents} which may
     * be a pending navigation or the last committed URL. For the last committed URL use
     * #getLastCommittedUrl().
     *
     * @return The URL of the current page or null if it's empty.
     */
    public String getUrl() {
        if (isDestroyed(WARN)) return null;
        String url = mWebContents.getVisibleUrl();
        if (url == null || url.trim().isEmpty()) return null;
        return url;
    }

    /**
     * Gets the last committed URL. It represents the current page that is
     * displayed in WebContents. It represents the current security context.
     *
     * @return The URL of the current page or null if it's empty.
     */
    public String getLastCommittedUrl() {
        if (isDestroyed(NO_WARN)) return null;
        String url = mWebContents.getLastCommittedUrl();
        if (url == null || url.trim().isEmpty()) return null;
        return url;
    }

    public void requestFocus() {
        mBisonViewMethods.requestFocus();
    }

    public void setBackgroundColor(int color) {
        mBaseBackgroundColor = color;
        if (!isDestroyed(WARN)) {
            BisonContentsJni.get().setBackgroundColor(mNativeBisonContents, BisonContents.this, color);
        }
    }

    /**
     * @see android.view.View#setLayerType()
     */
    public void setLayerType(int layerType, Paint paint) {
        mBisonViewMethods.setLayerType(layerType, paint);
    }

    int getEffectiveBackgroundColor() {
        // Do not ask the WebContents for the background color, as it will always
        // report white prior to initial navigation or post destruction,  whereas we want
        // to use the client supplied base value in those cases.
        if (isDestroyed(NO_WARN) || !mContentsClient.isCachedRendererBackgroundColorValid()) {
            return mBaseBackgroundColor;
        }
        return mContentsClient.getCachedRendererBackgroundColor();
    }

    public boolean isMultiTouchZoomSupported() {
        return mSettings.supportsMultiTouchZoom();
    }

    public View getZoomControlsForTest() {
        return mZoomControls.getZoomControlsViewForTest();
    }

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

    // TODO(mkosiba): In WebViewClassic these appear in some of the scroll extent calculation
    // methods but toggling them has no visiual effect on the content (in other words the scrolling
    // code behaves as if the scrollbar-related padding is in place but the onDraw code doesn't
    // take that into consideration).
    // http://crbug.com/269032
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
        if (TRACE) Log.i(TAG, "%s setHorizontalScrollbarOverlay=%s", this, overlay);
        mOverlayHorizontalScrollbar = overlay;
    }

    /**
     * @see View#setVerticalScrollbarOverlay(boolean)
     */
    public void setVerticalScrollbarOverlay(boolean overlay) {
        if (TRACE) Log.i(TAG, "%s setVerticalScrollbarOverlay=%s", this, overlay);
        mOverlayVerticalScrollbar = overlay;
    }

    /**
     * @see View#overlayHorizontalScrollbar()
     */
    public boolean overlayHorizontalScrollbar() {
        return mOverlayHorizontalScrollbar;
    }

    /**
     * @see View#overlayVerticalScrollbar()
     */
    public boolean overlayVerticalScrollbar() {
        return mOverlayVerticalScrollbar;
    }

    /**
     * Called by the embedder when the scroll offset of the containing view has changed.
     * @see View#onScrollChanged(int,int)
     */
    public void onContainerViewScrollChanged(int l, int t, int oldl, int oldt) {
        mBisonViewMethods.onContainerViewScrollChanged(l, t, oldl, oldt);
    }

    /**
     * Called by the embedder when the containing view is to be scrolled or overscrolled.
     * @see View#onOverScrolled(int,int,int,int)
     */
    public void onContainerViewOverScrolled(int scrollX, int scrollY, boolean clampedX,
            boolean clampedY) {
        mBisonViewMethods.onContainerViewOverScrolled(scrollX, scrollY, clampedX, clampedY);
    }

    /**
     * @see android.webkit.WebView#requestChildRectangleOnScreen(View, Rect, boolean)
     */
    public boolean requestChildRectangleOnScreen(View child, Rect rect, boolean immediate) {
        if (isDestroyed(WARN)) return false;
        return mScrollOffsetManager.requestChildRectangleOnScreen(
                child.getLeft() - child.getScrollX(), child.getTop() - child.getScrollY(),
                rect, immediate);
    }

    /**
     * @see View#computeHorizontalScrollRange()
     */
    public int computeHorizontalScrollRange() {
        return mBisonViewMethods.computeHorizontalScrollRange();
    }

    /**
     * @see View#computeHorizontalScrollOffset()
     */
    public int computeHorizontalScrollOffset() {
        return mBisonViewMethods.computeHorizontalScrollOffset();
    }

    /**
     * @see View#computeVerticalScrollRange()
     */
    public int computeVerticalScrollRange() {
        return mBisonViewMethods.computeVerticalScrollRange();
    }

    /**
     * @see View#computeVerticalScrollOffset()
     */
    public int computeVerticalScrollOffset() {
        return mBisonViewMethods.computeVerticalScrollOffset();
    }

    /**
     * @see View#computeVerticalScrollExtent()
     */
    public int computeVerticalScrollExtent() {
        return mBisonViewMethods.computeVerticalScrollExtent();
    }

    /**
     * @see View.computeScroll()
     */
    public void computeScroll() {
        mBisonViewMethods.computeScroll();
    }

    /**
     * @see View#onCheckIsTextEditor()
     */
    public boolean onCheckIsTextEditor() {
        return mBisonViewMethods.onCheckIsTextEditor();
    }

    /**
     * @see android.webkit.WebView#stopLoading()
     */
    public void stopLoading() {
        if (TRACE) Log.i(TAG, "%s stopLoading", this);
        if (!isDestroyed(WARN)) mWebContents.stop();
    }

    /**
     * @see android.webkit.WebView#reload()
     */
    public void reload() {
        if (TRACE) Log.i(TAG, "%s reload", this);
        if (!isDestroyed(WARN)) mNavigationController.reload(true);
    }

    /**
     * @see android.webkit.WebView#canGoBack()
     */
    public boolean canGoBack() {
        return isDestroyed(WARN) ? false : mNavigationController.canGoBack();
    }

    /**
     * @see android.webkit.WebView#goBack()
     */
    public void goBack() {
        if (TRACE) Log.i(TAG, "%s goBack", this);
        if (!isDestroyed(WARN)) mNavigationController.goBack();
    }

    /**
     * @see android.webkit.WebView#canGoForward()
     */
    public boolean canGoForward() {
        return isDestroyed(WARN) ? false : mNavigationController.canGoForward();
    }

    /**
     * @see android.webkit.WebView#goForward()
     */
    public void goForward() {
        if (TRACE) Log.i(TAG, "%s goForward", this);
        if (!isDestroyed(WARN)) mNavigationController.goForward();
    }

    /**
     * @see android.webkit.WebView#canGoBackOrForward(int)
     */
    public boolean canGoBackOrForward(int steps) {
        return isDestroyed(WARN) ? false : mNavigationController.canGoToOffset(steps);
    }

    /**
     * @see android.webkit.WebView#goBackOrForward(int)
     */
    public void goBackOrForward(int steps) {
        if (TRACE) Log.i(TAG, "%s goBackOrForwad=%d", this, steps);
        if (!isDestroyed(WARN)) mNavigationController.goToOffset(steps);
    }

    /**
     * @see android.webkit.WebView#pauseTimers()
     */
    public void pauseTimers() {
        if (TRACE) Log.i(TAG, "%s pauseTimers", this);
        if (!isDestroyed(WARN)) {
            ContentViewStatics.setWebKitSharedTimersSuspended(true);
        }
    }

    /**
     * @see android.webkit.WebView#resumeTimers()
     */
    public void resumeTimers() {
        if (TRACE) Log.i(TAG, "%s resumeTimers", this);
        if (!isDestroyed(WARN)) {
            ContentViewStatics.setWebKitSharedTimersSuspended(false);
        }
    }

    /**
     * @see android.webkit.WebView#onPause()
     */
    public void onPause() {
        if (TRACE) Log.i(TAG, "%s onPause", this);
        if (mIsPaused || isDestroyed(NO_WARN)) return;
        mIsPaused = true;
        BisonContentsJni.get().setIsPaused(mNativeBisonContents, BisonContents.this, mIsPaused);

        // Geolocation is paused/resumed via the page visibility mechanism.
        updateWebContentsVisibility();
    }

    /**
     * @see android.webkit.WebView#onResume()
     */
    public void onResume() {
        if (TRACE) Log.i(TAG, "%s onResume", this);
        if (!mIsPaused || isDestroyed(NO_WARN)) return;
        mIsPaused = false;
        BisonContentsJni.get().setIsPaused(mNativeBisonContents, BisonContents.this, mIsPaused);
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
        return mBisonViewMethods.onCreateInputConnection(outAttrs);
    }

    /**
     * @see android.webkit.WebView#onDragEvent(DragEvent)
     */
    public boolean onDragEvent(DragEvent event) {
        return mBisonViewMethods.onDragEvent(event);
    }

    /**
     * @see android.webkit.WebView#onKeyUp(int, KeyEvent)
     */
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        return mBisonViewMethods.onKeyUp(keyCode, event);
    }

    /**
     * @see android.webkit.WebView#dispatchKeyEvent(KeyEvent)
     */
    public boolean dispatchKeyEvent(KeyEvent event) {
        return mBisonViewMethods.dispatchKeyEvent(event);
    }

    /**
     * Clears the resource cache. Note that the cache is per-application, so this will clear the
     * cache for all WebViews used.
     *
     * @param includeDiskFiles if false, only the RAM cache is cleared
     */
    public void clearCache(boolean includeDiskFiles) {
        if (TRACE) Log.i(TAG, "%s clearCache", this);
        if (!isDestroyed(WARN)) {
            BisonContentsJni.get().clearCache(mNativeBisonContents, BisonContents.this, includeDiskFiles);
        }
    }

    @VisibleForTesting
    public void killRenderProcess() {
        if (TRACE) Log.i(TAG, "%s killRenderProcess", this);
        if (isDestroyed(WARN)) {
            throw new IllegalStateException("killRenderProcess() shouldn't be invoked after render"
                    + " process is gone or webview is destroyed");
        }
        BisonContentsJni.get().killRenderProcess(mNativeBisonContents, BisonContents.this);
    }

    public void documentHasImages(Message message) {
        if (!isDestroyed(WARN)) {
            BisonContentsJni.get().documentHasImages(mNativeBisonContents, BisonContents.this, message);
        }
    }

    public void saveWebArchive(
            final String basename, boolean autoname, final Callback<String> callback) {
        if (TRACE) Log.i(TAG, "%s saveWebArchive=%s", this, basename);
        if (!autoname) {
            saveWebArchiveInternal(basename, callback);
            return;
        }
        // If auto-generating the file name, handle the name generation on a background thread
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
        }
                .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public String getOriginalUrl() {
        if (isDestroyed(WARN)) return null;
        NavigationHistory history = mNavigationController.getNavigationHistory();
        int currentIndex = history.getCurrentEntryIndex();
        if (currentIndex >= 0 && currentIndex < history.getEntryCount()) {
            return history.getEntryAtIndex(currentIndex).getOriginalUrl();
        }
        return null;
    }

    /**
     * @see NavigationController#getNavigationHistory()
     */
    public NavigationHistory getNavigationHistory() {
        return isDestroyed(WARN) ? null : mNavigationController.getNavigationHistory();
    }

    /**
     * @see android.webkit.WebView#getTitle()
     */
    public String getTitle() {
        return isDestroyed(WARN) ? null : mWebContents.getTitle();
    }

    /**
     * @see android.webkit.WebView#clearHistory()
     */
    public void clearHistory() {
        if (TRACE) Log.i(TAG, "%s clearHistory", this);
        if (!isDestroyed(WARN)) mNavigationController.clearHistory();
    }

    /**
     * @see android.webkit.WebView#getCertificate()
     */
    public SslCertificate getCertificate() {
        return isDestroyed(WARN)
                ? null
                : SslUtil.getCertificateFromDerBytes(
                        BisonContentsJni.get().getCertificate(mNativeBisonContents, BisonContents.this));
    }

    /**
     * @see android.webkit.WebView#clearSslPreferences()
     */
    public void clearSslPreferences() {
        if (TRACE) Log.i(TAG, "%s clearSslPreferences", this);
        if (!isDestroyed(WARN)) mNavigationController.clearSslPreferences();
    }

    /**
     * Method to return all hit test values relevant to public WebView API.
     * Note that this expose more data than needed for WebView.getHitTestResult.
     * Unsafely returning reference to mutable internal object to avoid excessive
     * garbage allocation on repeated calls.
     */
    public HitTestData getLastHitTestResult() {
        if (TRACE) Log.i(TAG, "%s getLastHitTestResult", this);
        if (isDestroyed(WARN)) return null;
        BisonContentsJni.get().updateLastHitTestData(mNativeBisonContents, BisonContents.this);
        return mPossiblyStaleHitTestData;
    }

    /**
     * @see android.webkit.WebView#requestFocusNodeHref()
     */
    public void requestFocusNodeHref(Message msg) {
        if (TRACE) Log.i(TAG, "%s requestFocusNodeHref", this);
        if (msg == null || isDestroyed(WARN)) return;

        BisonContentsJni.get().updateLastHitTestData(mNativeBisonContents, BisonContents.this);
        Bundle data = msg.getData();

        // In order to maintain compatibility with the old WebView's implementation,
        // the absolute (full) url is passed in the |url| field, not only the href attribute.
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
        if (TRACE) Log.i(TAG, "%s requestImageRef", this);
        if (msg == null || isDestroyed(WARN)) return;

        BisonContentsJni.get().updateLastHitTestData(mNativeBisonContents, BisonContents.this);
        Bundle data = msg.getData();
        data.putString("url", mPossiblyStaleHitTestData.imgSrc);
        msg.setData(data);
        msg.sendToTarget();
    }

    @VisibleForTesting
    public float getPageScaleFactor() {
        return mPageScaleFactor;
    }

    private float getDeviceScaleFactor() {
        return mWindowAndroid.getWindowAndroid().getDisplay().getDipScale();
    }

    /**
     * Add the {@link WebMessageListener} to BisonContents, it will also inject the JavaScript object
     * with the given name to frames that have origins matching the allowedOriginRules. Note that
     * this call will not inject the JS object immediately. The JS object will be injected only for
     * future navigations (in DidClearWindowObject).
     *
     * @param jsObjectName    The name for the injected JavaScript object for this {@link
     *                        WebMessageListener}.
     * @param allowedOrigins  A list of matching rules for the allowed origins.
     *                        The JavaScript object will be injected when the frame's origin matches
     *                        any one of the allowed origins. If a wildcard "*" is provided, it will
     *                        inject JavaScript object to all frames.
     * @param listener        The {@link WebMessageListener} to be called when received
     *                        onPostMessage().
     * @throws IllegalArgumentException if one of the allowedOriginRules is invalid or one of
     *                                  jsObjectName and allowedOriginRules is {@code null}.
     * @throws NullPointerException if listener is {@code null}.
     */
    public void addWebMessageListener(@NonNull String jsObjectName,
            @NonNull String[] allowedOriginRules, @NonNull WebMessageListener listener) {
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
                BisonContentsJni.get().addWebMessageListener(mNativeBisonContents, BisonContents.this,
                        new WebMessageListenerHolder(listener), jsObjectName, allowedOriginRules);

        if (!TextUtils.isEmpty(exceptionMessage)) {
            throw new IllegalArgumentException(exceptionMessage);
        }
    }

    /**
     * Removes the {@link WebMessageListener} added by {@link addWebMessageListener}. This call will
     * immediately remove the JavaScript object/WebMessageListener mapping pair. So any messages
     * from the JavaScript object will be dropped. However the JavaScript object will only be
     * removed for future navigations.
     *
     * @param listener The {@link WebMessageListener} to be removed. Can not be {@code null}.
     */
    public void removeWebMessageListener(@NonNull String jsObjectName) {
        BisonContentsJni.get().removeWebMessageListener(
                mNativeBisonContents, BisonContents.this, jsObjectName);
    }

    /**
     * @see android.webkit.WebView#getScale()
     *
     * Please note that the scale returned is the page scale multiplied by
     * the screen density factor. See CTS WebViewTest.testSetInitialScale.
     */
    public float getScale() {
        if (isDestroyed(WARN)) return 1;
        return mPageScaleFactor * getDeviceScaleFactor();
    }

    /**
     * @see android.webkit.WebView#flingScroll(int, int)
     */
    public void flingScroll(int velocityX, int velocityY) {
        if (TRACE) Log.i(TAG, "%s flingScroll", this);
        if (isDestroyed(WARN)) return;
        mWebContents.getEventForwarder().startFling(
                SystemClock.uptimeMillis(), -velocityX, -velocityY, false, true);
    }

    /**
     * @see android.webkit.WebView#pageUp(boolean)
     */
    public boolean pageUp(boolean top) {
        if (TRACE) Log.i(TAG, "%s pageUp", this);
        if (isDestroyed(WARN)) return false;
        return mScrollOffsetManager.pageUp(top);
    }

    /**
     * @see android.webkit.WebView#pageDown(boolean)
     */
    public boolean pageDown(boolean bottom) {
        if (TRACE) Log.i(TAG, "%s pageDown", this);
        if (isDestroyed(WARN)) return false;
        return mScrollOffsetManager.pageDown(bottom);
    }

    /**
     * @see android.webkit.WebView#canZoomIn()
     */
    // This method uses the term 'zoom' for legacy reasons, but relates
    // to what chrome calls the 'page scale factor'.
    public boolean canZoomIn() {
        if (isDestroyed(WARN)) return false;
        final float zoomInExtent = mMaxPageScaleFactor - mPageScaleFactor;
        return zoomInExtent > ZOOM_CONTROLS_EPSILON;
    }

    /**
     * @see android.webkit.WebView#canZoomOut()
     */
    // This method uses the term 'zoom' for legacy reasons, but relates
    // to what chrome calls the 'page scale factor'.
    public boolean canZoomOut() {
        if (isDestroyed(WARN)) return false;
        final float zoomOutExtent = mPageScaleFactor - mMinPageScaleFactor;
        return zoomOutExtent > ZOOM_CONTROLS_EPSILON;
    }

    /**
     * @see android.webkit.WebView#zoomIn()
     */
    // This method uses the term 'zoom' for legacy reasons, but relates
    // to what chrome calls the 'page scale factor'.
    public boolean zoomIn() {
        if (!canZoomIn()) {
            return false;
        }
        zoomBy(1.25f);
        return true;
    }

    /**
     * @see android.webkit.WebView#zoomOut()
     */
    // This method uses the term 'zoom' for legacy reasons, but relates
    // to what chrome calls the 'page scale factor'.
    public boolean zoomOut() {
        if (!canZoomOut()) {
            return false;
        }
        zoomBy(0.8f);
        return true;
    }

    /**
     * @see android.webkit.WebView#zoomBy()
     */
    // This method uses the term 'zoom' for legacy reasons, but relates
    // to what chrome calls the 'page scale factor'.
    public void zoomBy(float delta) {
        if (isDestroyed(WARN)) return;
        if (delta < 0.01f || delta > 100.0f) {
            throw new IllegalStateException("zoom delta value outside [0.01, 100] range.");
        }
        BisonContentsJni.get().zoomBy(mNativeBisonContents, BisonContents.this, delta);
    }

    /**
     * @see android.webkit.WebView#invokeZoomPicker()
     */
    public void invokeZoomPicker() {
        if (TRACE) Log.i(TAG, "%s invokeZoomPicker", this);
        if (!isDestroyed(WARN)) mZoomControls.invokeZoomPicker();
    }

    /**
     * @see android.webkit.WebView#preauthorizePermission(Uri, long)
     */
    public void preauthorizePermission(Uri origin, long resources) {
        if (isDestroyed(NO_WARN)) return;
        BisonContentsJni.get().preauthorizePermission(
                mNativeBisonContents, BisonContents.this, origin.toString(), resources);
    }

    /**
     * @see WebContents.evaluateJavaScript(String, JavaScriptCallback)
     */
    public void evaluateJavaScript(String script, final Callback<String> callback) {
        if (TRACE) Log.i(TAG, "%s evaluateJavascript=%s", this, script);
        if (isDestroyed(WARN)) return;
        JavaScriptCallback jsCallback = null;
        if (callback != null) {
            jsCallback = jsonResult -> {
                // Post the application callback back to the current thread to ensure the
                // application callback is executed without any native code on the stack. This
                // so that any exception thrown by the application callback won't have to be
                // propagated through a native call stack.
                PostTask.postTask(UiThreadTaskTraits.DEFAULT, () -> callback.onResult(jsonResult));
            };
        }

        mWebContents.evaluateJavaScript(script, jsCallback);
    }

    public void evaluateJavaScriptForTests(String script, final Callback<String> callback) {
        if (TRACE) Log.i(TAG, "%s evaluateJavascriptForTests=%s", this, script);
        if (isDestroyed(NO_WARN)) return;
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
            String message, String targetOrigin, MessagePort[] sentPorts) {
        if (isDestroyed(WARN)) return;

        RenderFrameHost mainFrame = mWebContents.getMainFrame();
        // If the RenderFrameHost or the RenderFrame doesn't exist we couldn't post the message.
        if (mainFrame == null || !mainFrame.isRenderFrameCreated()) return;

        mWebContents.postMessageToMainFrame(message, null, targetOrigin, sentPorts);
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
        if (isDestroyed(NO_WARN)) return false;
        return mWebContents.hasAccessedInitialDocument();
    }

    private WebContentsAccessibility getWebContentsAccessibility() {
        return WebContentsAccessibility.fromWebContents(mWebContents);
    }

    @TargetApi(Build.VERSION_CODES.M)
    public void onProvideVirtualStructure(ViewStructure structure) {
        if (isDestroyed(WARN)) return;
        if (!mWebContentsObserver.didEverCommitNavigation()) {
            // TODO(sgurun) write a test case for this condition crbug/605251
            structure.setChildCount(0);
            return;
        }
        // for webview, the platform already calculates the scroll (as it is a view) in
        // ViewStructure tree. Do not offset for it in the snapshop x,y position calculations.
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

    //--------------------------------------------------------------------------------------------
    //  View and ViewGroup method implementations
    //--------------------------------------------------------------------------------------------
    /**
     * Calls android.view.View#startActivityForResult.  A RuntimeException will
     * be thrown by Android framework if startActivityForResult is called with
     * a non-Activity context.
     */
    void startActivityForResult(Intent intent, int requestCode) {
        // Even in fullscreen mode, startActivityForResult will still use the
        // initial internal access delegate because it has access to
        // the hidden API View#startActivityForResult.
        mFullScreenTransitionsState.getInitialInternalAccessDelegate()
                .super_startActivityForResult(intent, requestCode);
    }

    void startProcessTextIntent(Intent intent) {
        // on Android M, WebView is not able to replace the text with the processed text.
        // So set the readonly flag for M.
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
        if (isDestroyed(NO_WARN)) return;
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
        return mBisonViewMethods.onTouchEvent(event);
    }

    /**
     * @see android.view.View#onHoverEvent()
     */
    public boolean onHoverEvent(MotionEvent event) {
        return mBisonViewMethods.onHoverEvent(event);
    }

    /**
     * @see android.view.View#onGenericMotionEvent()
     */
    public boolean onGenericMotionEvent(MotionEvent event) {
        return isDestroyed(NO_WARN) ? false : mBisonViewMethods.onGenericMotionEvent(event);
    }

    /**
     * @see android.view.View#onConfigurationChanged()
     */
    public void onConfigurationChanged(Configuration newConfig) {
        mBisonViewMethods.onConfigurationChanged(newConfig);
    }

    /**
     * @see android.view.View#onAttachedToWindow()
     */
    public void onAttachedToWindow() {
        if (TRACE) Log.i(TAG, "%s onAttachedToWindow", this);
        mTemporarilyDetached = false;
        mBisonViewMethods.onAttachedToWindow();
        mWindowAndroid.getWindowAndroid().getDisplay().addObserver(mDisplayObserver);
    }

    /**
     * @see android.view.View#onDetachedFromWindow()
     */
    @SuppressLint("MissingSuperCall")
    public void onDetachedFromWindow() {
        if (TRACE) Log.i(TAG, "%s onDetachedFromWindow", this);
        mWindowAndroid.getWindowAndroid().getDisplay().removeObserver(mDisplayObserver);
        mBisonViewMethods.onDetachedFromWindow();
    }

    /**
     * @see android.view.View#onWindowFocusChanged()
     */
    public void onWindowFocusChanged(boolean hasWindowFocus) {
        mBisonViewMethods.onWindowFocusChanged(hasWindowFocus);
    }

    /**
     * @see android.view.View#onFocusChanged()
     */
    public void onFocusChanged(boolean focused, int direction, Rect previouslyFocusedRect) {
        if (!mTemporarilyDetached) {
            mBisonViewMethods.onFocusChanged(focused, direction, previouslyFocusedRect);
        }
    }

    /**
     * @see android.view.View#onStartTemporaryDetach()
     */
    public void onStartTemporaryDetach() {
        mTemporarilyDetached = true;
    }

    /**
     * @see android.view.View#onFinishTemporaryDetach()
     */
    public void onFinishTemporaryDetach() {
        mTemporarilyDetached = false;
    }

    /**
     * @see android.view.View#onSizeChanged()
     */
    public void onSizeChanged(int w, int h, int ow, int oh) {
        mBisonViewMethods.onSizeChanged(w, h, ow, oh);
    }

    /**
     * @see android.view.View#onVisibilityChanged()
     */
    public void onVisibilityChanged(View changedView, int visibility) {
        mBisonViewMethods.onVisibilityChanged(changedView, visibility);
    }

    /**
     * @see android.view.View#onWindowVisibilityChanged()
     */
    public void onWindowVisibilityChanged(int visibility) {
        mBisonViewMethods.onWindowVisibilityChanged(visibility);
    }

    private void setViewVisibilityInternal(boolean visible) {
        mIsViewVisible = visible;
        if (!isDestroyed(NO_WARN)) {
            BisonContentsJni.get().setViewVisibility(
                    mNativeBisonContents, BisonContents.this, mIsViewVisible);
        }
        postUpdateWebContentsVisibility();
    }

    private void setWindowVisibilityInternal(boolean visible) {
        mInvalidateRootViewOnNextDraw |= Build.VERSION.SDK_INT <= Build.VERSION_CODES.LOLLIPOP
                && visible && !mIsWindowVisible;
        mIsWindowVisible = visible;
        if (!isDestroyed(NO_WARN)) {
            BisonContentsJni.get().setWindowVisibility(
                    mNativeBisonContents, BisonContents.this, mIsWindowVisible);
        }
        postUpdateWebContentsVisibility();
    }

    private void postUpdateWebContentsVisibility() {
        if (mIsUpdateVisibilityTaskPending) return;
        // When WebView is attached to a visible window, WebView will be
        // attached to a window whose visibility is initially invisible, then
        // the window visibility will be updated to true. This means CVC
        // visibility will be set to false then true immediately, in the same
        // function call of View#dispatchAttachedToWindow.  DetachedFromWindow
        // is a similar case, where window visibility changes before BisonContents
        // is detached from window.
        //
        // To prevent this flip of CVC visibility, post the task to update CVC
        // visibility during attach, detach and window visibility change.
        mIsUpdateVisibilityTaskPending = true;
        PostTask.postTask(UiThreadTaskTraits.DEFAULT, mUpdateVisibilityRunnable);
    }

    private void updateWebContentsVisibility() {
        mIsUpdateVisibilityTaskPending = false;
        if (isDestroyed(NO_WARN)) return;
        boolean contentVisible = BisonContentsJni.get().isVisible(mNativeBisonContents, BisonContents.this);

        if (contentVisible && !mIsContentVisible) {
            mWebContents.onShow();
        } else if (!contentVisible && mIsContentVisible) {
            mWebContents.onHide();
        }
        mIsContentVisible = contentVisible;
        updateChildProcessImportance();
    }

    /**
     * Returns true if the page is visible according to DOM page visibility API.
     * See http://www.w3.org/TR/page-visibility/
     * This method is only called by tests and will return the supposed CVC
     * visibility without waiting a pending mUpdateVisibilityRunnable to run.
     */
    @VisibleForTesting
    public boolean isPageVisible() {
        if (isDestroyed(NO_WARN)) return mIsContentVisible;
        return BisonContentsJni.get().isVisible(mNativeBisonContents, BisonContents.this);
    }

    /**
     * Key for opaque state in bundle. Note this is only public for tests.
     */
    public static final String SAVE_RESTORE_STATE_KEY = "WEBVIEW_CHROMIUM_STATE";

    /**
     * Save the state of this BisonContents into provided Bundle.
     * @return False if saving state failed.
     */
    public boolean saveState(Bundle outState) {
        if (TRACE) Log.i(TAG, "%s saveState", this);
        if (isDestroyed(WARN) || outState == null) return false;

        byte[] state = BisonContentsJni.get().getOpaqueState(mNativeBisonContents, BisonContents.this);
        if (state == null) return false;

        outState.putByteArray(SAVE_RESTORE_STATE_KEY, state);
        return true;
    }

    /**
     * Restore the state of this BisonContents into provided Bundle.
     * @param inState Must be a bundle returned by saveState.
     * @return False if restoring state failed.
     */
    public boolean restoreState(Bundle inState) {
        if (TRACE) Log.i(TAG, "%s restoreState", this);
        if (isDestroyed(WARN) || inState == null) return false;

        byte[] state = inState.getByteArray(SAVE_RESTORE_STATE_KEY);
        if (state == null) return false;

        boolean result = BisonContentsJni.get().restoreFromOpaqueState(
                mNativeBisonContents, BisonContents.this, state);

        // The onUpdateTitle callback normally happens when a page is loaded,
        // but is optimized out in the restoreState case because the title is
        // already restored. See WebContentsImpl::UpdateTitleForEntry. So we
        // call the callback explicitly here.
        if (result) mContentsClient.onReceivedTitle(mWebContents.getTitle());

        return result;
    }

    /**
     * @see JavascriptInjector#addPossiblyUnsafeInterface(Object, String, Class)
     */
    @SuppressLint("NewApi")  // JavascriptInterface requires API level 17.
    public void addJavascriptInterface(Object object, String name) {
        if (TRACE) Log.i(TAG, "%s addJavascriptInterface=%s", this, name);
        if (isDestroyed(WARN)) return;
        Class<? extends Annotation> requiredAnnotation = null;
        if (mAppTargetSdkVersion >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            requiredAnnotation = JavascriptInterface.class;
        }

        getJavascriptInjector().addPossiblyUnsafeInterface(object, name, requiredAnnotation);
    }

    /**
     * @see android.webkit.WebView#removeJavascriptInterface(String)
     */
    public void removeJavascriptInterface(String interfaceName) {
        if (TRACE) Log.i(TAG, "%s removeInterface=%s", this, interfaceName);
        if (isDestroyed(WARN)) return;

        getJavascriptInjector().removeInterface(interfaceName);
    }

    /**
     * If native accessibility (not script injection) is enabled, and if this is
     * running on JellyBean or later, returns an AccessibilityNodeProvider that
     * implements native accessibility for this view. Returns null otherwise.
     * @return The AccessibilityNodeProvider, if available, or null otherwise.
     */
    public AccessibilityNodeProvider getAccessibilityNodeProvider() {
        return mBisonViewMethods.getAccessibilityNodeProvider();
    }

    public boolean supportsAccessibilityAction(int action) {
        return isDestroyed(WARN) ? false : getWebContentsAccessibility().supportsAction(action);
    }

    /**
     * @see android.webkit.WebView#performAccessibilityAction(int, Bundle)
     */
    public boolean performAccessibilityAction(int action, Bundle arguments) {
        return mBisonViewMethods.performAccessibilityAction(action, arguments);
    }

    /**
     * @see android.webkit.WebView#clearFormData()
     */
    public void hideAutofillPopup() {
        if (TRACE) Log.i(TAG, "%s hideAutofillPopup", this);
        if (mBisonAutofillClient != null) {
            mBisonAutofillClient.hideAutofillPopup();
        }
    }

    public void setNetworkAvailable(boolean networkUp) {
        if (TRACE) Log.i(TAG, "%s setNetworkAvailable=%s", this, networkUp);
        if (!isDestroyed(WARN)) {
            // For backward compatibility when an app uses this API disable the
            // Network Information API to prevent inconsistencies,
            // see crbug.com/520088.
            NetworkChangeNotifier.setAutoDetectConnectivityState(false);
            BisonContentsJni.get().setJsOnlineProperty(mNativeBisonContents, BisonContents.this, networkUp);
        }
    }

    /**
     * Inserts a {@link VisualStateCallback}.
     *
     * The {@link VisualStateCallback} will be inserted in Blink and will be invoked when the
     * contents of the DOM tree at the moment that the callback was inserted (or later) are drawn
     * into the screen. In other words, the following events need to happen before the callback is
     * invoked:
     * 1. The DOM tree is committed becoming the pending tree - see ThreadProxy::BeginMainFrame
     * 2. The pending tree is activated becoming the active tree
     * 3. A frame swap happens that draws the active tree into the screen
     *
     * @param requestId an id that will be returned from the callback invocation to allow
     * callers to match requests with callbacks.
     * @param callback the callback to be inserted
     * @throw IllegalStateException if this method is invoked after {@link #destroy()} has been
     * called.
     */
    public void insertVisualStateCallback(long requestId, VisualStateCallback callback) {
        if (TRACE) Log.i(TAG, "%s insertVisualStateCallback", this);
        if (isDestroyed(NO_WARN)) throw new IllegalStateException(
                "insertVisualStateCallback cannot be called after the WebView has been destroyed");
        BisonContentsJni.get().insertVisualStateCallback(
                mNativeBisonContents, BisonContents.this, requestId, callback);
    }

    public boolean isPopupWindow() {
        return mIsPopupWindow;
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

    public void setRendererPriorityPolicy(
            @RendererPriority int rendererRequestedPriority, boolean waivedWhenNotVisible) {
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

    public BisonRenderProcess getRenderProcess() {
        if (isDestroyed(WARN)) {
            return null;
        }
        return BisonContentsJni.get().getRenderProcess(mNativeBisonContents, BisonContents.this);
    }

    //--------------------------------------------------------------------------------------------
    //  Methods called from native via JNI
    //--------------------------------------------------------------------------------------------

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

    @CalledByNative
    private long onCreateTouchHandle() {
        PopupTouchHandleDrawable drawable = PopupTouchHandleDrawable.create(
                mTouchHandleDrawables, mWebContents, mContainerView);
        return drawable.getNativeDrawable();
    }

    /** Callback for generateMHTML. */
    @CalledByNative
    private static void generateMHTMLCallback(String path, long size, Callback<String> callback) {
        if (callback == null) return;
        callback.onResult(size < 0 ? null : path);
    }

    @CalledByNative
    private void onReceivedHttpAuthRequest(BisonHttpAuthHandler handler, String host, String realm) {
        mContentsClient.onReceivedHttpAuthRequest(handler, host, realm);

        BisonHistogramRecorder.recordCallbackInvocation(
                BisonHistogramRecorder.WebViewCallbackType.ON_RECEIVED_HTTP_AUTH_REQUEST);
    }

    public BisonGeolocationPermissions getGeolocationPermissions() {
        return mBrowserContext.getGeolocationPermissions();
    }

    public void invokeGeolocationCallback(boolean value, String requestingFrame) {
        if (isDestroyed(NO_WARN)) return;
        BisonContentsJni.get().invokeGeolocationCallback(
                mNativeBisonContents, BisonContents.this, value, requestingFrame);
    }

    @CalledByNative
    private void onGeolocationPermissionsShowPrompt(String origin) {
        if (isDestroyed(NO_WARN)) return;
        BisonGeolocationPermissions permissions = mBrowserContext.getGeolocationPermissions();
        // Reject if geoloaction is disabled, or the origin has a retained deny
        if (!mSettings.getGeolocationEnabled()) {
            BisonContentsJni.get().invokeGeolocationCallback(
                    mNativeBisonContents, BisonContents.this, false, origin);
            return;
        }
        // Allow if the origin has a retained allow
        if (permissions.hasOrigin(origin)) {
            BisonContentsJni.get().invokeGeolocationCallback(mNativeBisonContents, BisonContents.this,
                    permissions.isOriginAllowed(origin), origin);
            return;
        }
        mContentsClient.onGeolocationPermissionsShowPrompt(
                origin, new BisonGeolocationCallback(origin, this));
    }

    @CalledByNative
    private void onGeolocationPermissionsHidePrompt() {
        mContentsClient.onGeolocationPermissionsHidePrompt();
    }

    @CalledByNative
    private void onPermissionRequest(BisonPermissionRequest awPermissionRequest) {
        mContentsClient.onPermissionRequest(awPermissionRequest);
    }

    @CalledByNative
    private void onPermissionRequestCanceled(BisonPermissionRequest awPermissionRequest) {
        mContentsClient.onPermissionRequestCanceled(awPermissionRequest);
    }

    @CalledByNative
    public void onFindResultReceived(int activeMatchOrdinal, int numberOfMatches,
            boolean isDoneCounting) {
        mContentsClient.onFindResultReceived(activeMatchOrdinal, numberOfMatches, isDoneCounting);
    }

    @CalledByNative
    public void onNewPicture() {
        // Don't call capturePicture() here but instead defer it until the posted task runs within
        // the callback helper, to avoid doubling back into the renderer compositor in the middle
        // of the notification it is sending up to here.
        mContentsClient.getCallbackHelper().postOnNewPicture(mPictureListenerContentProvider);
    }

    /**
     * Invokes the given {@link VisualStateCallback}.
     *
     * @param callback the callback to be invoked
     * @param requestId the id passed to {@link BisonContents#insertVisualStateCallback}
     * @param result true if the callback should succeed and false otherwise
     */
    @CalledByNative
    public void invokeVisualStateCallback(
            final VisualStateCallback callback, final long requestId) {
        if (isDestroyed(NO_WARN)) return;
        // Posting avoids invoking the callback inside invoking_composite_
        // (see synchronous_compositor_impl.cc and crbug/452530).
        PostTask.postTask(UiThreadTaskTraits.DEFAULT, () -> callback.onComplete(requestId));
    }

    // Called as a result of BisonContentsJni.get().updateLastHitTestData.
    @CalledByNative
    private void updateHitTestData(
            int type, String extra, String href, String anchorText, String imgSrc) {
        mPossiblyStaleHitTestData.hitTestResultType = type;
        mPossiblyStaleHitTestData.hitTestResultExtraData = extra;
        mPossiblyStaleHitTestData.href = href;
        mPossiblyStaleHitTestData.anchorText = anchorText;
        mPossiblyStaleHitTestData.imgSrc = imgSrc;
    }

    @CalledByNative
    private void postInvalidateOnAnimation() {
        if (!mWindowAndroid.getWindowAndroid().isInsideVSync()) {
            mContainerView.postInvalidateOnAnimation();
        } else {
            mContainerView.invalidate();
        }
    }

    @CalledByNative
    private int[] getLocationOnScreen() {
        int[] result = new int[2];
        mContainerView.getLocationOnScreen(result);
        return result;
    }

    @CalledByNative
    private void onWebLayoutPageScaleFactorChanged(float webLayoutPageScaleFactor) {
        // This change notification comes from the renderer thread, not from the cc/ impl thread.
        mLayoutSizer.onPageScaleChanged(webLayoutPageScaleFactor);
    }

    @CalledByNative
    private void onWebLayoutContentsSizeChanged(int widthCss, int heightCss) {
        // This change notification comes from the renderer thread, not from the cc/ impl thread.
        mLayoutSizer.onContentSizeChanged(widthCss, heightCss);
    }

    @CalledByNative
    private void scrollContainerViewTo(int xPx, int yPx) {
        // Both xPx and yPx are in physical pixel scale.
        mScrollOffsetManager.scrollContainerViewTo(xPx, yPx);
    }

    @CalledByNative
    private void updateScrollState(int maxContainerViewScrollOffsetX,
            int maxContainerViewScrollOffsetY, float contentWidthDip, float contentHeightDip,
            float pageScaleFactor, float minPageScaleFactor, float maxPageScaleFactor) {
        mContentWidthDip = contentWidthDip;
        mContentHeightDip = contentHeightDip;
        mScrollOffsetManager.setMaxScrollOffset(maxContainerViewScrollOffsetX,
                maxContainerViewScrollOffsetY);
        setPageScaleFactorAndLimits(pageScaleFactor, minPageScaleFactor, maxPageScaleFactor);
    }

    @CalledByNative
    private void setBisonAutofillClient(BisonAutofillClient client) {
        mBisonAutofillClient = client;
        client.init(mContext);
    }

    @CalledByNative
    private void didOverscroll(int deltaX, int deltaY, float velocityX, float velocityY) {
        mScrollOffsetManager.overScrollBy(deltaX, deltaY);

        if (mOverScrollGlow == null) return;

        mOverScrollGlow.setOverScrollDeltas(deltaX, deltaY);
        final int oldX = mContainerView.getScrollX();
        final int oldY = mContainerView.getScrollY();
        final int x = oldX + deltaX;
        final int y = oldY + deltaY;
        final int scrollRangeX = mScrollOffsetManager.computeMaximumHorizontalScrollOffset();
        final int scrollRangeY = mScrollOffsetManager.computeMaximumVerticalScrollOffset();
        // absorbGlow() will release the glow if it is not finished.
        mOverScrollGlow.absorbGlow(x, y, oldX, oldY, scrollRangeX, scrollRangeY,
                (float) Math.hypot(velocityX, velocityY));

        if (mOverScrollGlow.isAnimating()) {
            postInvalidateOnAnimation();
        }
    }

    /**
     * Determine if at least one edge of the WebView extends over the edge of the window.
     */
    private boolean extendsOutOfWindow() {
        int loc[] = new int[2];
        mContainerView.getLocationOnScreen(loc);
        int x = loc[0];
        int y = loc[1];
        mContainerView.getRootView().getLocationOnScreen(loc);
        int rootX = loc[0];
        int rootY = loc[1];

        // Get the position of the current view, relative to its root view
        int relativeX = x - rootX;
        int relativeY = y - rootY;

        if (relativeX < 0 || relativeY < 0
                || relativeX + mContainerView.getWidth() > mContainerView.getRootView().getWidth()
                || relativeY + mContainerView.getHeight()
                        > mContainerView.getRootView().getHeight()) {
            return true;
        }
        return false;
    }

    /**
     * Determine if it's reasonable to show any sort of interstitial. If the WebView is not visible,
     * the user may not be able to interact with the UI.
     * @return true if the WebView is visible
     */
    @VisibleForTesting
    @CalledByNative
    protected boolean canShowInterstitial() {
        return mIsAttachedToWindow && mIsViewVisible;
    }

    @CalledByNative
    private int getErrorUiType() {
        if (extendsOutOfWindow()) {
            return ErrorUiType.QUIET_GIANT;
        } else if (canShowBigInterstitial()) {
            return ErrorUiType.LOUD;
        } else {
            return ErrorUiType.QUIET_SMALL;
        }
    }

    /**
     * Determine if it's suitable to show the interstitial for browsers and main UIs. If the WebView
     * is close to full-screen, we assume the app is using it as the main UI, so we show the same
     * interstitial Chrome uses. Otherwise, we assume the WebView is part of a larger composed page,
     * and will show a different interstitial.
     * @return true if the WebView should display the large interstitial
     */
    @VisibleForTesting
    protected boolean canShowBigInterstitial() {
        double percentOfScreenHeight =
                (double) mContainerView.getHeight() / mContainerView.getRootView().getHeight();

        // Make a guess as to whether the WebView is the predominant part of the UI
        return mContainerView.getWidth() == mContainerView.getRootView().getWidth()
                && percentOfScreenHeight >= MIN_SCREEN_HEIGHT_PERCENTAGE_FOR_INTERSTITIAL;
    }

    @VisibleForTesting
    public void evaluateJavaScriptOnInterstitialForTesting(
            String script, final Callback<String> callback) {
        if (TRACE) Log.i(TAG, "%s evaluateJavaScriptOnInterstitialForTesting=%s", this, script);
        if (isDestroyed(WARN)) return;
        JavaScriptCallback jsCallback = null;
        if (callback != null) {
            jsCallback = jsonResult -> callback.onResult(jsonResult);
        }

        BisonContentsJni.get().evaluateJavaScriptOnInterstitialForTesting(
                mNativeBisonContents, BisonContents.this, script, jsCallback);
    }

    @CalledByNative
    private static void onEvaluateJavaScriptResultForTesting(
            String jsonResult, JavaScriptCallback callback) {
        callback.handleJavaScriptResult(jsonResult);
    }

    /**
     * Return the device locale in the same format we use to populate the 'hl' query parameter for
     * Safe Browsing interstitial urls, as done in BaseUIManager::app_locale().
     */
    @VisibleForTesting
    public static String getSafeBrowsingLocaleForTesting() {
        return BisonContentsJni.get().getSafeBrowsingLocaleForTesting();
    }

    // -------------------------------------------------------------------------------------------
    // Helper methods
    // -------------------------------------------------------------------------------------------

    private void setPageScaleFactorAndLimits(
            float pageScaleFactor, float minPageScaleFactor, float maxPageScaleFactor) {
        if (mPageScaleFactor == pageScaleFactor
                && mMinPageScaleFactor == minPageScaleFactor
                && mMaxPageScaleFactor == maxPageScaleFactor) {
            return;
        }
        mMinPageScaleFactor = minPageScaleFactor;
        mMaxPageScaleFactor = maxPageScaleFactor;
        if (mPageScaleFactor != pageScaleFactor) {
            float oldPageScaleFactor = mPageScaleFactor;
            mPageScaleFactor = pageScaleFactor;
            float dipScale = getDeviceScaleFactor();
            mContentsClient.getCallbackHelper().postOnScaleChangedScaled(
                    oldPageScaleFactor * dipScale, mPageScaleFactor * dipScale);
        }
    }

    private void saveWebArchiveInternal(String path, final Callback<String> callback) {
        if (path == null || isDestroyed(WARN)) {
            if (callback == null) return;

            PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> callback.onResult(null));
        } else {
            BisonContentsJni.get().generateMHTML(mNativeBisonContents, BisonContents.this, path, callback);
        }
    }

    /**
     * Try to generate a pathname for saving an MHTML archive. This roughly follows WebView's
     * autoname logic.
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

        if (TextUtils.isEmpty(name)) name = "index";

        String testName = baseName + name + WEB_ARCHIVE_EXTENSION;
        if (!new File(testName).exists()) return testName;

        for (int i = 1; i < 100; i++) {
            testName = baseName + name + "-" + i + WEB_ARCHIVE_EXTENSION;
            if (!new File(testName).exists()) return testName;
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
        if (isDestroyed(WARN)) return;

        mWebContents.setSmartClipResultHandler(resultHandler);
    }

    protected void insertVisualStateCallbackIfNotDestroyed(
            long requestId, VisualStateCallback callback) {
        if (TRACE) Log.i(TAG, "%s insertVisualStateCallbackIfNotDestroyed", this);
        if (isDestroyed(NO_WARN)) return;
        BisonContentsJni.get().insertVisualStateCallback(
                mNativeBisonContents, BisonContents.this, requestId, callback);
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

    // --------------------------------------------------------------------------------------------
    // This is the BisonViewMethods implementation that does real work. The BisonViewMethodsImpl is
    // hooked up to the WebView in embedded mode and to the FullScreenView in fullscreen mode,
    // but not to both at the same time.
    private class BisonViewMethodsImpl implements BisonViewMethods {
        private int mLayerType = View.LAYER_TYPE_NONE;
        private ComponentCallbacks2 mComponentCallbacks;

        // Only valid within software onDraw().
        private final Rect mClipBoundsTemporary = new Rect();

        @SuppressLint("DrawAllocation") // For new BisonFunctor.
        @Override
        public void onDraw(Canvas canvas) {
            if (isDestroyed(NO_WARN)) {
                TraceEvent.instant("EarlyOut_destroyed");
                canvas.drawColor(getEffectiveBackgroundColor());
                return;
            }

            // For hardware draws, the clip at onDraw time could be different
            // from the clip during DrawGL.
            if (!canvas.isHardwareAccelerated() && !canvas.getClipBounds(mClipBoundsTemporary)) {
                TraceEvent.instant("EarlyOut_software_empty_clip");
                return;
            }

            if (canvas.isHardwareAccelerated() && mDrawFunctor == null) {

                BisonFunctor newFunctor;
                BisonDrawFnImpl.DrawFnAccess drawFnAccess =
                        mNativeDrawFunctorFactory.getDrawFnAccess();

                if (drawFnAccess != null) {
                    newFunctor = new BisonDrawFnImpl(drawFnAccess);
                } else {
                    newFunctor = new BisonGLFunctor(mNativeDrawFunctorFactory, mContainerView);
                }
                setFunctor(newFunctor);
            }

            mScrollOffsetManager.syncScrollOffsetFromOnDraw();
            int scrollX = mContainerView.getScrollX();
            int scrollY = mContainerView.getScrollY();
            Rect globalVisibleRect = getGlobalVisibleRect();
            // Workaround for bug in libhwui on N that does not swap if inserting functor is the
            // only operation in a canvas. See crbug.com/704212.
            if (Build.VERSION.SDK_INT == Build.VERSION_CODES.N
                    || Build.VERSION.SDK_INT == Build.VERSION_CODES.N_MR1) {
                if (mPaintForNWorkaround == null) {
                    mPaintForNWorkaround = new Paint();
                    // Note a completely transparent color will get optimized out. So draw almost
                    // transparent black, but then scale alpha down to effectively 0.
                    mPaintForNWorkaround.setColor(Color.argb(1, 0, 0, 0));
                    ColorMatrix colorMatrix = new ColorMatrix();
                    colorMatrix.setScale(0.f, 0.f, 0.f, 0.1f);
                    mPaintForNWorkaround.setColorFilter(new ColorMatrixColorFilter(colorMatrix));
                }
                canvas.drawRect(0, 0, 1, 1, mPaintForNWorkaround);
            }
            boolean did_draw = BisonContentsJni.get().onDraw(mNativeBisonContents, BisonContents.this,
                    canvas, canvas.isHardwareAccelerated(), scrollX, scrollY,
                    globalVisibleRect.left, globalVisibleRect.top, globalVisibleRect.right,
                    globalVisibleRect.bottom, ForceAuxiliaryBitmapRendering.sResult);
            if (canvas.isHardwareAccelerated()
                    && BisonContentsJni.get().needToDrawBackgroundColor(
                            mNativeBisonContents, BisonContents.this)) {
                TraceEvent.instant("DrawBackgroundColor");
                canvas.drawColor(getEffectiveBackgroundColor());
            }
            if (did_draw && canvas.isHardwareAccelerated()
                    && !ForceAuxiliaryBitmapRendering.sResult) {
                did_draw = mDrawFunctor.requestDraw(canvas);
            }
            if (did_draw) {
                int scrollXDiff = mContainerView.getScrollX() - scrollX;
                int scrollYDiff = mContainerView.getScrollY() - scrollY;
                canvas.translate(-scrollXDiff, -scrollYDiff);
            } else {
                TraceEvent.instant("NativeDrawFailed");
                canvas.drawColor(getEffectiveBackgroundColor());
            }

            if (mOverScrollGlow != null && mOverScrollGlow.drawEdgeGlows(canvas,
                    mScrollOffsetManager.computeMaximumHorizontalScrollOffset(),
                    mScrollOffsetManager.computeMaximumVerticalScrollOffset())) {
                postInvalidateOnAnimation();
            }

            if (mInvalidateRootViewOnNextDraw) {
                mContainerView.getRootView().invalidate();
                mInvalidateRootViewOnNextDraw = false;
            }
        }

        @Override
        public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            mLayoutSizer.onMeasure(widthMeasureSpec, heightMeasureSpec);
        }

        @Override
        public void requestFocus() {
            if (isDestroyed(NO_WARN)) return;
            if (!mContainerView.isInTouchMode() && mSettings.shouldFocusFirstNode()) {
                BisonContentsJni.get().focusFirstNode(mNativeBisonContents, BisonContents.this);
            }
        }

        @Override
        public void setLayerType(int layerType, Paint paint) {
            mLayerType = layerType;
            updateHardwareAcceleratedFeaturesToggle();
        }

        private void updateHardwareAcceleratedFeaturesToggle() {
            mSettings.setEnableSupportedHardwareAcceleratedFeatures(
                    mIsAttachedToWindow && mContainerView.isHardwareAccelerated()
                            && (mLayerType == View.LAYER_TYPE_NONE
                                    || mLayerType == View.LAYER_TYPE_HARDWARE));
        }

        @Override
        public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
            return isDestroyed(NO_WARN)
                    ? null
                    : ImeAdapter.fromWebContents(mWebContents).onCreateInputConnection(outAttrs);
        }

        @Override
        public boolean onDragEvent(DragEvent event) {
            return isDestroyed(NO_WARN)
                    ? false
                    : mWebContents.getEventForwarder().onDragEvent(event, mContainerView);
        }

        @Override
        public boolean onKeyUp(int keyCode, KeyEvent event) {
            return isDestroyed(NO_WARN) ? false
                                        : mWebContents.getEventForwarder().onKeyUp(keyCode, event);
        }

        @Override
        public boolean dispatchKeyEvent(KeyEvent event) {
            if (isDestroyed(NO_WARN)) return false;
            if (isDpadEvent(event)) {
                mSettings.setSpatialNavigationEnabled(true);
            }

            // Following check is dup'ed from |ContentUiEventHandler.dispatchKeyEvent| to avoid
            // embedder-specific customization, which is necessary only for WebView.
            if (GamepadList.dispatchKeyEvent(event)) return true;

            // This check reflects Chrome's behavior and is a workaround for http://b/7697782.
            if (mContentsClient.hasWebViewClient()
                    && mContentsClient.shouldOverrideKeyEvent(event)) {
                return mInternalAccessAdapter.super_dispatchKeyEvent(event);
            }
            return mWebContents.getEventForwarder().dispatchKeyEvent(event);
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if (isDestroyed(NO_WARN)) return false;
            if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
                mSettings.setSpatialNavigationEnabled(false);
            }

            mScrollOffsetManager.setProcessingTouchEvent(true);
            boolean rv = mWebContents.getEventForwarder().onTouchEvent(event);
            mScrollOffsetManager.setProcessingTouchEvent(false);

            if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
                // Note this will trigger IPC back to browser even if nothing is
                // hit.
                float eventX = event.getX();
                float eventY = event.getY();
                float touchMajor = Math.max(event.getTouchMajor(), event.getTouchMinor());
                if (!UseZoomForDSFPolicy.isUseZoomForDSFEnabled()) {
                    float dipScale = getDeviceScaleFactor();
                    eventX /= dipScale;
                    eventY /= dipScale;
                    touchMajor /= dipScale;
                }
                BisonContentsJni.get().requestNewHitTestDataAt(
                        mNativeBisonContents, BisonContents.this, eventX, eventY, touchMajor);
            }

            if (mOverScrollGlow != null) {
                if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
                    mOverScrollGlow.setShouldPull(true);
                } else if (event.getActionMasked() == MotionEvent.ACTION_UP
                        || event.getActionMasked() == MotionEvent.ACTION_CANCEL) {
                    mOverScrollGlow.setShouldPull(false);
                    mOverScrollGlow.releaseAll();
                }
            }

            return rv;
        }

        @Override
        public boolean onHoverEvent(MotionEvent event) {
            return isDestroyed(NO_WARN) ? false
                                        : mWebContents.getEventForwarder().onHoverEvent(event);
        }

        @Override
        public boolean onGenericMotionEvent(MotionEvent event) {
            return isDestroyed(NO_WARN)
                    ? false
                    : mWebContents.getEventForwarder().onGenericMotionEvent(event);
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
            if (isDestroyed(NO_WARN)) return;
            if (mIsAttachedToWindow) {
                Log.w(TAG, "onAttachedToWindow called when already attached. Ignoring");
                return;
            }
            mIsAttachedToWindow = true;

            mViewEventSink.onAttachedToWindow();
            BisonContentsJni.get().onAttachedToWindow(mNativeBisonContents, BisonContents.this,
                    mContainerView.getWidth(), mContainerView.getHeight());
            updateHardwareAcceleratedFeaturesToggle();
            postUpdateWebContentsVisibility();

            updateDefaultLocale();

            if (mComponentCallbacks != null) return;
            mComponentCallbacks = new BisonComponentCallbacks();
            mContext.registerComponentCallbacks(mComponentCallbacks);
        }

        @Override
        public void onDetachedFromWindow() {
            if (isDestroyed(NO_WARN)) return;
            if (!mIsAttachedToWindow) {
                Log.w(TAG, "onDetachedFromWindow called when already detached. Ignoring");
                return;
            }
            mIsAttachedToWindow = false;
            hideAutofillPopup();
            BisonContentsJni.get().onDetachedFromWindow(mNativeBisonContents, BisonContents.this);

            mViewEventSink.onDetachedFromWindow();
            updateHardwareAcceleratedFeaturesToggle();
            postUpdateWebContentsVisibility();
            setFunctor(null);

            if (mComponentCallbacks != null) {
                mContext.unregisterComponentCallbacks(mComponentCallbacks);
                mComponentCallbacks = null;
            }

            mScrollAccessibilityHelper.removePostedCallbacks();
            mZoomControls.dismissZoomPicker();
        }

        @Override
        public void onWindowFocusChanged(boolean hasWindowFocus) {
            if (isDestroyed(NO_WARN)) return;
            mWindowFocused = hasWindowFocus;
            mViewEventSink.onWindowFocusChanged(hasWindowFocus);
            Clipboard.getInstance().onWindowFocusChanged(hasWindowFocus);
        }

        @Override
        public void onFocusChanged(boolean focused, int direction, Rect previouslyFocusedRect) {
            if (isDestroyed(NO_WARN)) return;
            mContainerViewFocused = focused;
            mViewEventSink.onViewFocusChanged(focused);
        }

        @Override
        public void onSizeChanged(int w, int h, int ow, int oh) {
            if (isDestroyed(NO_WARN)) return;
            mScrollOffsetManager.setContainerViewSize(w, h);
            // The BisonLayoutSizer needs to go first so that if we're in
            // fixedLayoutSize mode the update
            // to enter fixedLayoutSize mode is sent before the first resize
            // update.
            mLayoutSizer.onSizeChanged(w, h, ow, oh);
            BisonContentsJni.get().onSizeChanged(mNativeBisonContents, BisonContents.this, w, h, ow, oh);
        }

        @Override
        public void onVisibilityChanged(View changedView, int visibility) {
            boolean viewVisible = mContainerView.getVisibility() == View.VISIBLE;
            if (mIsViewVisible == viewVisible) return;
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
            // A side-effect of View.onScrollChanged is that the scroll accessibility event being
            // sent by the base class implementation. This is completely hidden from the base
            // classes and cannot be prevented, which is why we need the code below.
            mScrollAccessibilityHelper.removePostedViewScrolledAccessibilityEventCallback();
            mScrollOffsetManager.onContainerViewScrollChanged(l, t);
        }

        @Override
        public void onContainerViewOverScrolled(int scrollX, int scrollY, boolean clampedX,
                boolean clampedY) {
            int oldX = mContainerView.getScrollX();
            int oldY = mContainerView.getScrollY();

            mScrollOffsetManager.onContainerViewOverScrolled(scrollX, scrollY, clampedX, clampedY);

            if (mOverScrollGlow != null) {
                mOverScrollGlow.pullGlow(mContainerView.getScrollX(), mContainerView.getScrollY(),
                        oldX, oldY,
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
            BisonContentsJni.get().onComputeScroll(mNativeBisonContents, BisonContents.this,
                    AnimationUtils.currentAnimationTimeMillis());
        }

        @Override
        public boolean onCheckIsTextEditor() {
            if (isDestroyed(NO_WARN)) return false;
            ImeAdapter imeAdapter = ImeAdapter.fromWebContents(mWebContents);
            return imeAdapter != null ? imeAdapter.onCheckIsTextEditor() : false;
        }

        @Override
        public AccessibilityNodeProvider getAccessibilityNodeProvider() {
            if (isDestroyed(NO_WARN)) return null;
            WebContentsAccessibility wcax = getWebContentsAccessibility();
            return wcax != null ? wcax.getAccessibilityNodeProvider() : null;
        }

        @Override
        public boolean performAccessibilityAction(final int action, final Bundle arguments) {
            if (isDestroyed(NO_WARN)) return false;
            WebContentsAccessibility wcax = getWebContentsAccessibility();
            return wcax != null ? wcax.performAction(action, arguments) : false;
        }
    }

    // Return true if the GeolocationPermissionAPI should be used.
    @CalledByNative
    private boolean useLegacyGeolocationPermissionAPI() {
        // Always return true since we are not ready to swap the geolocation yet.
        // TODO: If we decide not to migrate the geolocation, there are some unreachable
        // code need to remove. http://crbug.com/396184.
        return true;
    }

    @NativeMethods
    interface Natives {
        long init(long browserContextPointer);

        void destroy(long nativeBisonContents);
        boolean hasRequiredHardwareExtensions();
        void setBisonDrawSWFunctionTable(long functionTablePointer);
        void setBisonDrawGLFunctionTable(long functionTablePointer);
        int getNativeInstanceCount();
        void setShouldDownloadFavicons();
        void updateDefaultLocale(String locale, String localeList);
        String getSafeBrowsingLocaleForTesting();
        void evaluateJavaScriptOnInterstitialForTesting(long nativeBisonContents, BisonContents caller,
                String script, JavaScriptCallback jsCallback);
        void setJavaPeers(long nativeBisonContents, BisonContents caller, BisonContents awContents,
                          BisonWebContentsDelegate webViewWebContentsDelegate,
                          BisonContentsClientBridge contentsClientBridge,
                          BisonContentsIoThreadClient ioThreadClient,
                          InterceptNavigationDelegate navigationInterceptionDelegate,
                          AutofillProvider autofillProvider);
        WebContents getWebContents(long nativeBisonContents, BisonContents caller);
        BisonBrowserContext getBrowserContext(long nativeBisonContents, BisonContents caller);
        void setCompositorFrameConsumer(
                long nativeBisonContents, BisonContents caller, long nativeCompositorFrameConsumer);
        void documentHasImages(long nativeBisonContents, BisonContents caller, Message message);
        void generateMHTML(
                long nativeBisonContents, BisonContents caller, String path, Callback<String> callback);
        void addVisitedLinks(long nativeBisonContents, BisonContents caller, String[] visitedLinks);
        void zoomBy(long nativeBisonContents, BisonContents caller, float delta);
        void onComputeScroll(
                long nativeBisonContents, BisonContents caller, long currentAnimationTimeMillis);
        boolean onDraw(long nativeBisonContents, BisonContents caller, Canvas canvas,
                       boolean isHardwareAccelerated, int scrollX, int scrollY, int visibleLeft,
                       int visibleTop, int visibleRight, int visibleBottom,
                       boolean forceAuxiliaryBitmapRendering);
        boolean needToDrawBackgroundColor(long nativeBisonContents, BisonContents caller);
        void findAllAsync(long nativeBisonContents, BisonContents caller, String searchString);
        void findNext(long nativeBisonContents, BisonContents caller, boolean forward);
        void clearMatches(long nativeBisonContents, BisonContents caller);
        void clearCache(long nativeBisonContents, BisonContents caller, boolean includeDiskFiles);
        void killRenderProcess(long nativeBisonContents, BisonContents caller);
        byte[] getCertificate(long nativeBisonContents, BisonContents caller);
        // Coordinates are in physical pixels when --use-zoom-for-dsf is enabled.
        // Otherwise, coordinates are in desity independent pixels.
        void requestNewHitTestDataAt(
                long nativeBisonContents, BisonContents caller, float x, float y, float touchMajor);

        void updateLastHitTestData(long nativeBisonContents, BisonContents caller);
        void onSizeChanged(long nativeBisonContents, BisonContents caller, int w, int h, int ow, int oh);
        void scrollTo(long nativeBisonContents, BisonContents caller, int x, int y);
        void restoreScrollAfterTransition(long nativeBisonContents, BisonContents caller, int x, int y);
        void smoothScroll(long nativeBisonContents, BisonContents caller, int targetX, int targetY,
                          long durationMs);
        void setViewVisibility(long nativeBisonContents, BisonContents caller, boolean visible);
        void setWindowVisibility(long nativeBisonContents, BisonContents caller, boolean visible);
        void setIsPaused(long nativeBisonContents, BisonContents caller, boolean paused);
        void onAttachedToWindow(long nativeBisonContents, BisonContents caller, int w, int h);
        void onDetachedFromWindow(long nativeBisonContents, BisonContents caller);
        boolean isVisible(long nativeBisonContents, BisonContents caller);
        void setDipScale(long nativeBisonContents, BisonContents caller, float dipScale);
        // Returns null if save state fails.
        byte[] getOpaqueState(long nativeBisonContents, BisonContents caller);

        // Returns false if restore state fails.
        boolean restoreFromOpaqueState(long nativeBisonContents, BisonContents caller, byte[] state);

        long releasePopupBisonContents(long nativeBisonContents, BisonContents caller);
        void focusFirstNode(long nativeBisonContents, BisonContents caller);
        void setBackgroundColor(long nativeBisonContents, BisonContents caller, int color);
        long capturePicture(long nativeBisonContents, BisonContents caller, int width, int height);
        void enableOnNewPicture(long nativeBisonContents, BisonContents caller, boolean enabled);
        void insertVisualStateCallback(long nativeBisonContents, BisonContents caller, long requestId,
                                       VisualStateCallback callback);
        void clearView(long nativeBisonContents, BisonContents caller);
        void setExtraHeadersForUrl(
                long nativeBisonContents, BisonContents caller, String url, String extraHeaders);
        void invokeGeolocationCallback(
                long nativeBisonContents, BisonContents caller, boolean value, String requestingFrame);
        int getEffectivePriority(long nativeBisonContents, BisonContents caller);
        void setJsOnlineProperty(long nativeBisonContents, BisonContents caller, boolean networkUp);
        void trimMemory(long nativeBisonContents, BisonContents caller, int level, boolean visible);
        void createPdfExporter(
                long nativeBisonContents, BisonContents caller, BisonPdfExporter awPdfExporter);
        void preauthorizePermission(
                long nativeBisonContents, BisonContents caller, String origin, long resources);
        void grantFileSchemeAccesstoChildProcess(long nativeBisonContents, BisonContents caller);
        void resumeLoadingCreatedPopupWebContents(long nativeBisonContents, BisonContents caller);
        BisonRenderProcess getRenderProcess(long nativeBisonContents, BisonContents caller);
        String addWebMessageListener(long nativeBisonContents, BisonContents caller,
                WebMessageListenerHolder listener, String jsObjectName, String[] allowedOrigins);
        void removeWebMessageListener(
                long nativeBisonContents, BisonContents caller, String jsObjectName);
    }
}
