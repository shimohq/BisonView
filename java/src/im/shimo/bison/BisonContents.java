package im.shimo.bison;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Message;
import android.text.TextUtils;
import android.util.Base64;
import android.view.KeyEvent;
import android.view.ViewGroup;
import android.webkit.JavascriptInterface;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.View;
import android.widget.FrameLayout;

import androidx.annotation.VisibleForTesting;
import androidx.annotation.NonNull;

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
import org.chromium.components.navigation_interception.InterceptNavigationDelegate;
import org.chromium.components.navigation_interception.NavigationParams;
import org.chromium.content_public.browser.ChildProcessImportance;
import org.chromium.content_public.browser.ImeAdapter;
import org.chromium.content_public.browser.JavaScriptCallback;
import org.chromium.content_public.browser.JavascriptInjector;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.NavigationController;
import org.chromium.content_public.browser.NavigationHistory;
import org.chromium.content_public.browser.SelectionPopupController;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsInternals;
import org.chromium.content_public.browser.navigation_controller.LoadURLType;
import org.chromium.content_public.browser.navigation_controller.UserAgentOverrideOption;
import org.chromium.content_public.common.ContentUrlConstants;
import org.chromium.content_public.common.Referrer;
import org.chromium.network.mojom.ReferrerPolicy;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.PageTransition;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.display.DisplayAndroid.DisplayAndroidObserver;
import org.chromium.url.GURL;

import java.io.File;
import java.lang.annotation.Annotation;
import java.lang.ref.WeakReference;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.WeakHashMap;
import java.util.regex.Pattern;

@JNINamespace("bison")
class BisonContents extends FrameLayout {
    private static final String TAG = "BisonContents";
    private static final boolean TRACE = true;
    private static final int NO_WARN = 0;
    private static final int WARN = 1;
    private static final String PRODUCT_VERSION = BisonContentsJni.get().getProductVersion();

    private static final String WEB_ARCHIVE_EXTENSION = ".mht";
    // The request code should be unique per BisonView/BisonContents object.
    private static final int PROCESS_TEXT_REQUEST_CODE = 100;

    private static String sCurrentLocales = "";
    private static final float ZOOM_CONTROLS_EPSILON = 0.007f;





    private static final Pattern sFileAndroidAssetPattern =
            Pattern.compile("^file:/*android_(asset|res).*");























    private long mNativeBisonContents;
    private BisonBrowserContext mBrowserContext;
    private ContentView mContentView;
    private ViewGroup mContainerView;
    private WindowAndroidWrapper mWindowAndroid;
    private WebContents mWebContents;
    private WebContentsInternalsHolder mWebContentsInternalsHolder;
    private NavigationController mNavigationController;
    private final BisonContentsClient mContentsClient;
    private BisonWebContentsObserver mWebContentsObserver;
    private final BisonContentsClientBridge mBisonContentsClientBridge;
    private BisonWebContentsDelegate mWebContentsDelegate;
    private final BisonContentsBackgroundThreadClient mBackgroundThreadClient;
    private final BisonContentsIoThreadClient mIoThreadClient;
    private final InterceptNavigationDelegateImpl mInterceptNavigationDelegate;
    private final BisonLayoutSizer mLayoutSizer;
    private final DisplayAndroidObserver mDisplayObserver;
    private BisonSettings mSettings;


    private boolean mIsPaused;
    private boolean mIsViewVisible;
    private boolean mIsContentVisible;
    private @RendererPriority int mRendererPriority;
    private boolean mRendererPriorityWaivedWhenNotVisible;

    private ContentViewRenderView mContentViewRenderView;

    private BisonViewAndroidDelegate mViewAndroidDelegate;


    private BisonViewClient mBisonViewClient;
    private BisonWebChromeClient mBisonWebChromeClient;
    private float mPageScaleFactor = 1.0f;
    private float mMinPageScaleFactor = 1.0f;
    private float mMaxPageScaleFactor = 1.0f;





    private BisonAutofillClient mAutofillClient;

    private BisonPdfExporter mBisonPdfExporter;
    private boolean mIsDestroyed;

    private AutofillProvider mAutofillProvider;
    private WebContentsInternals mWebContentsInternals;

    private JavascriptInjector mJavascriptInjector;

    private boolean mIsUpdateVisibilityTaskPending;
    private Runnable mUpdateVisibilityRunnable;

    private static class WebContentsInternalsHolder implements WebContents.InternalsHolder {
        private final WeakReference<BisonContents> mBisonContentsRef;

        private WebContentsInternalsHolder(BisonContents bisonContents) {
            mBisonContentsRef = new WeakReference<>(bisonContents);
        }

        @Override
        public void set(WebContentsInternals internals) {
            BisonContents bisonContents = mBisonContentsRef.get();
            if (bisonContents == null) {
                throw new IllegalStateException("AwContents should be available at this time");
            }
            bisonContents.mWebContentsInternals = internals;
        }

        @Override
        public WebContentsInternals get() {
            BisonContents bisonContents = mBisonContentsRef.get();
            return bisonContents == null ? null : bisonContents.mWebContentsInternals;
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
        public BisonWebResourceResponse shouldInterceptRequest(BisonContentsClient.BisonWebResourceRequest request) {
            String url = request.url;
            BisonWebResourceResponse webResourceResponse;
            // Return the response directly if the url is default video poster url.
            //webResourceResponse = mDefaultVideoPosterRequestHandler.shouldInterceptRequest(url);
            //if (webResourceResponse != null) return webResourceResponse;

            webResourceResponse = mContentsClient.shouldInterceptRequest(request);

            if (webResourceResponse == null) {
                mContentsClient.postOnLoadResource(url);
            }

            //if (webResourceResponse != null) {
            //    String mimeType = webResourceResponse.getMimeType();
            //    if (mimeType == null) {
            //        AwHistogramRecorder.recordMimeType(
            //                AwHistogramRecorder.MimeType.NULL_FROM_SHOULD_INTERCEPT_REQUEST);
            //    } else {
            //        AwHistogramRecorder.recordMimeType(
            //                AwHistogramRecorder.MimeType.NONNULL_FROM_SHOULD_INTERCEPT_REQUEST);
            //    }
            //}
            if (webResourceResponse != null && webResourceResponse.getData() == null) {
                // In this case the intercepted URLRequest job will simulate an empty response
                // which doesn't trigger the onReceivedError callback. For WebViewClassic
                // compatibility we synthesize that callback.  http://crbug.com/180950
                mContentsClient.postOnReceivedError(
                        request, new BisonContentsClient.BisonWebResourceError());
            }
            return webResourceResponse;
        }
    }


    private class InterceptNavigationDelegateImpl implements InterceptNavigationDelegate {
        @Override
        public boolean shouldIgnoreNavigation(NavigationParams navigationParams) {
            if (!navigationParams.isRendererInitiated) {
                mContentsClient.getCallbackHelper().postOnPageStarted(navigationParams.url);
            }
            return false;
        }
    }

    //--------------------------------------------------------------------------------------------
    private class LayoutSizerDelegate implements BisonLayoutSizer.Delegate {
        @Override
        public void requestLayout() {
            BisonContents.this.requestLayout();
        }

        @Override
        public void setMeasuredDimension(int measuredWidth, int measuredHeight) {
            //mInternalAccessAdapter.setMeasuredDimension(measuredWidth, measuredHeight);
        }

        @Override
        public boolean isLayoutParamsHeightWrapContent() {
            // return mContainerView.getLayoutParams() != null
            //         && (mContainerView.getLayoutParams().height
            //                 == ViewGroup.LayoutParams.WRAP_CONTENT);
            // jiang mContainerView is webView ?
            return getLayoutParams() != null
                    && (getLayoutParams().height
                    == ViewGroup.LayoutParams.WRAP_CONTENT);
        }

        @Override
        public void setForceZeroLayoutHeight(boolean forceZeroHeight) {
            getSettings().setForceZeroLayoutHeight(forceZeroHeight);
        }
    }

    //--------------------------------------------------------------------------------------------



















    //--------------------------------------------------------------------------------------------
    private class BisonDisplayAndroidObserver implements DisplayAndroidObserver {
        @Override
        public void onRotationChanged(int rotation) {
        }

        @Override
        public void onDIPScaleChanged(float dipScale) {
            if (TRACE) Log.i(TAG, "%s onDIPScaleChanged dipScale=%f", this, dipScale);

            BisonContentsJni.get().setDipScale(mNativeBisonContents, BisonContents.this, dipScale);
            mLayoutSizer.setDIPScale(dipScale);
            mSettings.setDIPScale(dipScale);
        }
    }

    //--------------------------------------------------------------------------------------------
    public BisonContents(Context context, ViewGroup containerView, BisonBrowserContext bisonBrowserContext,
                         BisonContentsClientBridge bisonContentsClientBridge,
                         BisonContentsClient bisonContentsClient) {
        super(context);
        mRendererPriority = RendererPriority.HIGH;
        mSettings = new BisonSettings(context);
        updateDefaultLocale();

        mBrowserContext = bisonBrowserContext;

        mNativeBisonContents = BisonContentsJni.get().init(this, mBrowserContext.getNativePointer());
        mWebContents = BisonContentsJni.get().getWebContents(mNativeBisonContents);
        mContainerView = containerView;

        mWindowAndroid = getWindowAndroid(context);
        mContentViewRenderView = new ContentViewRenderView(getContext());
        mContentViewRenderView.onNativeLibraryLoaded(mWindowAndroid.getWindowAndroid());
        //mWindowAndroid.getWindowAndroid().setAnimationPlaceholderView(mContentViewRenderView.getSurfaceView());

        addView(mContentViewRenderView);

        mContentView = ContentView.createContentView(context,null, mWebContents ,mContainerView);
        mViewAndroidDelegate = new BisonViewAndroidDelegate(mContentView);
        mWebContentsInternalsHolder = new WebContentsInternalsHolder(this);

        mWebContents.initialize(
                PRODUCT_VERSION, mViewAndroidDelegate, mContentView, mWindowAndroid.getWindowAndroid(), mWebContentsInternalsHolder);
        SelectionPopupController.fromWebContents(mWebContents)
                .setActionModeCallback(new BisonActionModeCallback(context, this, mWebContents));

        mNavigationController = mWebContents.getNavigationController();
        if (getParent() != null) mWebContents.onShow();
        addView(mContentView);
        mContentView.requestFocus();
        mContentViewRenderView.setCurrentWebContents(mWebContents);


        mBisonContentsClientBridge = bisonContentsClientBridge;
        //mAutofillProvider
        mContentsClient = bisonContentsClient;
        mBackgroundThreadClient = new BackgroundThreadClientImpl();
        mIoThreadClient = new IoThreadClientImpl();
        mInterceptNavigationDelegate = new InterceptNavigationDelegateImpl();

        mLayoutSizer = new BisonLayoutSizer();
        mLayoutSizer.setDelegate(new LayoutSizerDelegate());
        mWebContentsDelegate = new BisonWebContentsDelegate(context, this, mContentsClient);
        mDisplayObserver = new BisonDisplayAndroidObserver();

        BisonContentsJni.get().setJavaPeers(mNativeBisonContents, mWebContentsDelegate,
                mBisonContentsClientBridge, mIoThreadClient, mInterceptNavigationDelegate, mAutofillProvider);
        installWebContentsObserver();
        mSettings.setWebContents(mWebContents);

        mDisplayObserver.onDIPScaleChanged(getDeviceScaleFactor());

        mUpdateVisibilityRunnable = () -> updateWebContentsVisibility();
        mCleanupReference = new CleanupReference(
                this, new BisonContentsDestroyRunnable(mNativeBisonContents, mWindowAndroid));
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
    // jiang  webview 加个开关
    // private static WeakHashMap<Context, WindowAndroidWrapper> sContextWindowMap;

    // getWindowAndroid is only called on UI thread, so there are no threading issues with lazy
    // initialization.
    private static WindowAndroidWrapper getWindowAndroid(Context context) {
        // if (sContextWindowMap == null) sContextWindowMap = new WeakHashMap<>();
        // WindowAndroidWrapper wrapper = sContextWindowMap.get(context);

        WindowAndroidWrapper wrapper = null;

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
            // sContextWindowMap.put(context, wrapper);
        }
        return wrapper;
    }

    private void setWebContents(long newBisonContentsPtr) {
       if (mNativeBisonContents != 0) {
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
        mWebContentsObserver = new BisonWebContentsObserver(mWebContents, this, mContentsClient);
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

    @CalledByNativeUnchecked
    protected boolean onRenderProcessGone(int childProcessID, boolean crashed) {
        if (isDestroyed(NO_WARN)) return false;
        return mContentsClient.onRenderProcessGone(new BisonRenderProcessGoneDetail(crashed,
                BisonContentsJni.get().getEffectivePriority(mNativeBisonContents, this)));
    }

    /**
     * Destroys this object and deletes its native counterpart.
     */
    public void destroy() {
        if (TRACE) Log.i(TAG, "%s destroy", this);
        if (isDestroyed(NO_WARN)) return;
        removeAllViews();
        if (mContentViewRenderView != null) {
            mContentViewRenderView.destroy();
            mContentViewRenderView= null;
        }

        // Remove pending messages
        mContentsClient.getCallbackHelper().removeCallbacksAndMessages();
        mIsDestroyed = true;
        PostTask.postTask(UiThreadTaskTraits.DEFAULT, () -> destroyNatives());
    }

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
    protected void onDestroyed() {
    }

    private boolean isDestroyed(int warnIfDestroyed) {
        if (mIsDestroyed && warnIfDestroyed == WARN) {
            Log.w(TAG, "Application attempted to call on a destroyed BisonView", new Throwable());
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

    public BisonSettings getSettings() {
        return mSettings;
    }

    public BisonPdfExporter getPdfExporter() {
        if (isDestroyed(WARN)) return null;
        if (mBisonPdfExporter == null) {
            mBisonPdfExporter = new BisonPdfExporter();
            BisonContentsJni.get().createPdfExporter(
                    mNativeBisonContents, mBisonPdfExporter);
        }
        return mBisonPdfExporter;
    }

    public void findAllAsync(String searchString) {
        if (TRACE) Log.i(TAG, "%s findAllAsync", this);
        if (!isDestroyed(WARN)) {
            BisonContentsJni.get().findAllAsync(mNativeBisonContents, searchString);
        }
    }

    public void findNext(boolean forward) {
        if (TRACE) Log.i(TAG, "%s findNext", this);
        if (!isDestroyed(WARN)) {
            BisonContentsJni.get().findNext(mNativeBisonContents, forward);
        }
    }


    public void loadUrl(String url, Map<String, String> additionalHttpHeaders) {
        if (TRACE) Log.i(TAG, "%s loadUrl(extra headers)=%s", this, url);
        if (isDestroyed(WARN)) return;

        if (url == null) {
            return;
        }
        LoadUrlParams params = new LoadUrlParams(url, PageTransition.TYPED);
        if (additionalHttpHeaders != null) {
            params.setExtraHeaders(new HashMap<>(additionalHttpHeaders));
        }

        loadUrl(params);
    }

    public void loadUrl(String url) {
        if (TRACE) Log.i(TAG, "%s loadUrl=%s", this, url);
        if (isDestroyed(WARN)) return;
        if (url == null) return;
        loadUrl(url, null);
    }
    public void postUrl(String url, byte[] postData) {
        if (TRACE) Log.i(TAG, "%s postUrl=%s", this, url);
        if (isDestroyed(WARN)) return;
        LoadUrlParams params = LoadUrlParams.createLoadHttpPostParams(url, postData);
        Map<String, String> headers = new HashMap<>();
        headers.put("Content-Type", "application/x-www-form-urlencoded");
        params.setExtraHeaders(headers);
        loadUrl(params);
    }

    private void loadUrl(LoadUrlParams params) {
        if (params.getLoadUrlType() == LoadURLType.DATA && !params.isBaseUrlDataScheme()) {
            params.setCanLoadLocalResources(true);
            BisonContentsJni.get().grantFileSchemeAccesstoChildProcess(mNativeBisonContents);
        }

        if (params.getUrl() != null && params.getUrl().equals(mWebContents.getLastCommittedUrl())
                && params.getTransitionType() == PageTransition.TYPED) {
            params.setTransitionType(PageTransition.RELOAD);
        }
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

        params.setTransitionType(
                params.getTransitionType() | PageTransition.FROM_API);

        BisonContentsJni.get().setExtraHeadersForUrl(mNativeBisonContents,
                params.getUrl(), params.getExtraHttpRequestHeadersString());
        params.setExtraHeaders(new HashMap<String, String>());

        mNavigationController.loadUrl(params);

        // WebViewClassic的行为使用WebKit中的populateVisitedLinks回调。
        // Chromium不会使用此使用代码路径，也不会使用此行为的最佳模拟来在WebView的第一个URL加载上调用一次请求访问的链接。
        // if (!mHasRequestedVisitedHistoryFromClient) {
        //     mHasRequestedVisitedHistoryFromClient = true;
        //     requestVisitedHistoryFromClient();
        // }
        // mContentView.requestFocus();
    }

    public void loadData(String data, String mimeType, String encoding) {
        if (TRACE) Log.i(TAG, "%s loadData", this);
        if (isDestroyed(WARN)) return;
        loadUrl(LoadUrlParams.createLoadDataParams(
                fixupData(data), fixupMimeType(mimeType), isBase64Encoded(encoding)));
    }

    public void loadData(String baseUrl, String data,
                         String mimeType, String encoding, String historyUrl) {
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

    public void setBisonViewClient(BisonViewClient client) {
        this.mBisonViewClient = client;
    }

    public void setBisonWebChromeClient(BisonWebChromeClient client) {
        this.mBisonWebChromeClient = client;
    }

       @Override
    public void setBackgroundColor(int color) {
        super.setBackgroundColor(color);
        mContentViewRenderView.setBackgroundColor(color);
    }
    public void stopLoading() {
        if (TRACE) Log.i(TAG, "%s stopLoading", this);
        if (!isDestroyed(WARN)) mWebContents.stop();
    }

    public void reload() {
        if (TRACE) Log.i(TAG, "%s reload", this);
        if (!isDestroyed(WARN)) mNavigationController.reload(true);
    }

    public boolean canGoBack() {
        return isDestroyed(WARN) ? false : mNavigationController.canGoBack();
    }

    public void goBack() {
        if (TRACE) Log.i(TAG, "%s goBack", this);
        if (!isDestroyed(WARN)) mNavigationController.goBack();
    }

    public boolean canGoForward() {
        return isDestroyed(WARN) ? false : mNavigationController.canGoForward();
    }

    public void goForward() {
        if (TRACE) Log.i(TAG, "%s goForward", this);
        if (!isDestroyed(WARN)) mNavigationController.goForward();
    }

    public boolean canGoBackOrForward(int steps) {
        return isDestroyed(WARN) ? false : mNavigationController.canGoToOffset(steps);
    }

    public void goBackOrForward(int steps) {
        if (TRACE) Log.i(TAG, "%s goBackOrForwad=%d", this, steps);
        if (!isDestroyed(WARN)) mNavigationController.goToOffset(steps);
    }

      public void pauseTimers() {
        if (TRACE) Log.i(TAG, "%s pauseTimers", this);
        if (!isDestroyed(WARN)) {
            BisonBrowserContext.getDefault().pauseTimers();
        }
    }

    public void resumeTimers() {
        if (TRACE) Log.i(TAG, "%s resumeTimers", this);
        if (!isDestroyed(WARN)) {
            BisonBrowserContext.getDefault().resumeTimers();
        }
    }

    public void onPause() {
        if (TRACE) Log.i(TAG, "%s onPause", this);
        if (mIsPaused || isDestroyed(NO_WARN)) return;
        mIsPaused = true;

    }

    public void onResume() {
        if (TRACE) Log.i(TAG, "%s onResume", this);
        if (!mIsPaused || isDestroyed(NO_WARN)) return;
        mIsPaused = false;

        updateWebContentsVisibility();
    }


    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        return mContentView.createInputConnection(outAttrs);
    }
    public void clearCache(boolean includeDiskFiles) {
        if (TRACE) Log.i(TAG, "%s clearCache", this);
        if (!isDestroyed(WARN)) {
            BisonContentsJni.get().clearCache(mNativeBisonContents, includeDiskFiles);
        }
    }

    @VisibleForTesting
    public void killRenderProcess() {
        if (TRACE) Log.i(TAG, "%s killRenderProcess", this);
        if (isDestroyed(WARN)) {
            throw new IllegalStateException("killRenderProcess() shouldn't be invoked after render"
                    + " process is gone or bisonview is destroyed");
        }
        BisonContentsJni.get().killRenderProcess(mNativeBisonContents, this);
    }

    public void documentHasImages(Message message) {
        if (!isDestroyed(WARN)) {
            BisonContentsJni.get().documentHasImages(mNativeBisonContents, message);
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

    public String getTitle() {
        return isDestroyed(WARN) ? null : mWebContents.getTitle();
    }

    public void clearHistory() {
        if (TRACE) Log.i(TAG, "%s clearHistory", this);
        if (!isDestroyed(WARN)) mNavigationController.clearHistory();
    }

    // public SslCertificate getCertificate() {
    //     return isDestroyed(WARN)
    //             ? null
    //             : SslUtil.getCertificateFromDerBytes(
    //                     AwContentsJni.get().getCertificate(mNativeAwContents, AwContents.this));
    // }

    public void clearSslPreferences() {
        if (TRACE) Log.i(TAG, "%s clearSslPreferences", this);
        if (!isDestroyed(WARN)) mNavigationController.clearSslPreferences();
    }


    public String getUrl() {
        GURL url = mWebContents.getVisibleUrl();
        //if (url == null || url.trim().isEmpty()) return null;
        return url == null ? null : url.getSpec();
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

    // private static void recordBaseUrl(@UrlScheme int value) {
    //     RecordHistogram.recordEnumeratedHistogram(
    //             DATA_BASE_URL_SCHEME_HISTOGRAM_NAME, value, UrlScheme.COUNT);
    // }

    // private static void recordLoadUrlScheme(@UrlScheme int value) {
    //     RecordHistogram.recordEnumeratedHistogram(
    //             LOAD_URL_SCHEME_HISTOGRAM_NAME, value, UrlScheme.COUNT);
    // }


    @CalledByNative
    private void setAutofillClient(BisonAutofillClient client) {
        mAutofillClient = client;
        client.init(getContext());
    }

    @CalledByNative
    private void onNativeDestroyed() {
        mWindowAndroid = null;
        mNativeBisonContents = 0;
        mWebContents = null;
    }


    public static String sanitizeUrl(String url) {
        if (url == null) return null;
        if (url.startsWith("www.") || url.indexOf(":") == -1) url = "http://" + url;
        return url;
    }



    public boolean isSelectActionModeAllowed(int actionModeItem) {
        return (mSettings.getDisabledActionModeMenuItems() & actionModeItem) != actionModeItem;
    }

    public boolean canZoomIn() {
        if (isDestroyed(WARN)) return false;
        final float zoomInExtent = mMaxPageScaleFactor - mPageScaleFactor;
        return zoomInExtent > ZOOM_CONTROLS_EPSILON;
    }

    public boolean canZoomOut() {
        if (isDestroyed(WARN)) return false;
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
        if (isDestroyed(WARN)) return;
        if (delta < 0.01f || delta > 100.0f) {
            throw new IllegalStateException("zoom delta value outside [0.01, 100] range.");
        }
        BisonContentsJni.get().zoomBy(mNativeBisonContents, this, delta);
    }

    public void preauthorizePermission(Uri origin, long resources) {
        if (isDestroyed(NO_WARN)) return;
        BisonContentsJni.get().preauthorizePermission(
                mNativeBisonContents, this, origin.toString(), resources);
    }

    public void evaluateJavaScript(String script, Callback<String> callback) {
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

    void startActivityForResult(Intent intent, int requestCode) {
        // Even in fullscreen mode, startActivityForResult will still use the
        // initial internal access delegate because it has access to
        // the hidden API View#startActivityForResult.

        // mFullScreenTransitionsState.getInitialInternalAccessDelegate()
        //        .super_startActivityForResult(intent, requestCode);
    }

    void startProcessTextIntent(Intent intent) {
        // on Android M, WebView is not able to replace the text with the processed text.
        // So set the readonly flag for M.
        if (Build.VERSION.SDK_INT == Build.VERSION_CODES.M) {
            intent.putExtra(Intent.EXTRA_PROCESS_TEXT_READONLY, true);
        }
        Context context = getContext();
        if (ContextUtils.activityFromContext(context) == null) {
            context.startActivity(intent);
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











    @Override
    protected void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);

        if (visibility == View.GONE) {
            mWebContents.onHide();
        } else if (visibility == View.VISIBLE) {
            mWebContents.onShow();
        }

    }
    private void setViewVisibilityInternal(boolean visible) {
        mIsViewVisible = visible;
        postUpdateWebContentsVisibility();
    }



    private void postUpdateWebContentsVisibility() {
        if (mIsUpdateVisibilityTaskPending) return;
        mIsUpdateVisibilityTaskPending = true;
        PostTask.postTask(UiThreadTaskTraits.DEFAULT, mUpdateVisibilityRunnable);
    }

    private void updateWebContentsVisibility() {
        mIsUpdateVisibilityTaskPending = false;
        if (isDestroyed(NO_WARN)) return;
        //boolean contentVisible = BisonContentsJni.get().isVisible(mNativeBisonContents, this);

        if (!mIsContentVisible) {
            mWebContents.onShow();
        } else {
            mWebContents.onHide();
        }
        updateChildProcessImportance();
    }

    public void addJavascriptInterface(Object object, String name) {
        if (TRACE) Log.i(TAG, "%s addJavascriptInterface=%s", this, name);
        if (isDestroyed(WARN)) return;
        Class<? extends Annotation> requiredAnnotation = JavascriptInterface.class;
        getJavascriptInjector().addPossiblyUnsafeInterface(object, name, requiredAnnotation);
    }

    public void removeJavascriptInterface(String interfaceName) {
        if (TRACE) Log.i(TAG, "%s removeInterface=%s", this, interfaceName);
        if (isDestroyed(WARN)) return;

        getJavascriptInjector().removeInterface(interfaceName);
    }

    public void disableJavascriptInterfacesInspection() {
       if (!isDestroyed(WARN)) {
           getJavascriptInjector().setAllowInspection(false);
       }
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

    private float getDeviceScaleFactor() {
        return mWindowAndroid.getWindowAndroid().getDisplay().getDipScale();
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

    @CalledByNative
    private static void onDocumentHasImagesResponse(boolean result, Message message) {
        message.arg1 = result ? 1 : 0;
        message.sendToTarget();
    }

    /**
     * Callback for generateMHTML.
     */
    @CalledByNative
    private static void generateMHTMLCallback(String path, long size, Callback<String> callback) {
        if (callback == null) return;
        callback.onResult(size < 0 ? null : path);
    }


    public BisonGeolocationPermissions getGeolocationPermissions() {
        return mBrowserContext.getGeolocationPermissions();
    }

    public void invokeGeolocationCallback(boolean value, String requestingFrame) {
        if (isDestroyed(NO_WARN)) return;
        BisonContentsJni.get().invokeGeolocationCallback(
                mNativeBisonContents, value, requestingFrame);
    }

    @CalledByNative
    private void onGeolocationPermissionsShowPrompt(String origin) {
        if (isDestroyed(NO_WARN)) return;
        BisonGeolocationPermissions permissions = mBrowserContext.getGeolocationPermissions();
        // Reject if geoloaction is disabled, or the origin has a retained deny
        if (!mSettings.getGeolocationEnabled()) {
            BisonContentsJni.get().invokeGeolocationCallback(
                    mNativeBisonContents, false, origin);
            return;
        }
        // Allow if the origin has a retained allow
        if (permissions.hasOrigin(origin)) {
            BisonContentsJni.get().invokeGeolocationCallback(mNativeBisonContents,
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
    private void onPermissionRequest(BisonPermissionRequest permissionRequest) {
        mContentsClient.onPermissionRequest(permissionRequest);
    }

    @CalledByNative
    private void onPermissionRequestCanceled(BisonPermissionRequest permissionRequest) {
        mContentsClient.onPermissionRequestCanceled(permissionRequest);
    }

    @CalledByNative
    public void onFindResultReceived(int activeMatchOrdinal, int numberOfMatches,
            boolean isDoneCounting) {
        mContentsClient.onFindResultReceived(activeMatchOrdinal, numberOfMatches, isDoneCounting);
    }


    // @CalledByNative
    private int[] getLocationOnScreen() {
        int[] result = new int[2];
        getLocationOnScreen(result);
        return result;
    }

    @CalledByNative
    private void onWebLayoutPageScaleFactorChanged(float webLayoutPageScaleFactor) {
        // This change notification comes from the renderer thread, not from the cc/ impl thread.
        mLayoutSizer.onPageScaleChanged(webLayoutPageScaleFactor);
    }

    // @CalledByNative
    private void onWebLayoutContentsSizeChanged(int widthCss, int heightCss) {
        // This change notification comes from the renderer thread, not from the cc/ impl thread.
        mLayoutSizer.onContentSizeChanged(widthCss, heightCss);
    }


    private void saveWebArchiveInternal(String path, final Callback<String> callback) {
        if (path == null || isDestroyed(WARN)) {
            if (callback == null) return;

            PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> callback.onResult(null));
        } else {
            BisonContentsJni.get().generateMHTML(mNativeBisonContents, path, callback);
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



    @Override
    protected void onVisibilityChanged(@NonNull View changedView, int visibility) {
        super.onVisibilityChanged(changedView, visibility);
        boolean viewVisible = getVisibility() == View.VISIBLE;
        setViewVisibilityInternal(viewVisible);
    }
    @CalledByNative
    private boolean useLegacyGeolocationPermissionAPI() {
        // Always return true since we are not ready to swap the geolocation yet.
        // TODO: If we decide not to migrate the geolocation, there are some unreachable
        // code need to remove. http://crbug.com/396184.
        return true;
    }


    static void logCommandLineForDebugging() {
        BisonContentsJni.get().logCommandLineForDebugging();
    }

    @NativeMethods
    interface Natives {
        long init(BisonContents caller, long nativeBisonBrowserContext);

        void destroy(long nativeBisonContents);
        void updateDefaultLocale(String locale, String localeList);
        void setJavaPeers(long nativeBisonContents, BisonWebContentsDelegate webContentsDelegate,
                BisonContentsClientBridge bisonContentsClientBridge,
                BisonContentsIoThreadClient ioThreadClient,
                InterceptNavigationDelegate navigationInterceptionDelegate,
                AutofillProvider autofillProvider);
        WebContents getWebContents(long nativeBisonContents);
        void documentHasImages(long nativeBisonContents, Message message);
        void generateMHTML(
                long nativeBisonContents, String path, Callback<String> callback);
        void zoomBy(long nativeBisonContents, BisonContents caller, float delta);
        void findAllAsync(long nativeBisonContents, String searchString);
        void findNext(long nativeBisonContents, boolean forward);
        void clearCache(long nativeBisonContents, boolean includeDiskFiles);
        void killRenderProcess(long nativeBisonContents, BisonContents caller);

        void setDipScale(long nativeBisonContents, BisonContents caller, float dipScale);



        void setExtraHeadersForUrl(
                long nativeBisonContents, String url, String extraHeaders);
        void invokeGeolocationCallback(
                long nativeBisonContents, boolean value, String requestingFrame);
        int getEffectivePriority(long nativeBisonContents, BisonContents caller);
        void createPdfExporter(
                long nativeBisonContents, BisonPdfExporter bisonPdfExporter);
        void preauthorizePermission(
                long nativeBisonContents, BisonContents caller, String origin, long resources);
        void grantFileSchemeAccesstoChildProcess(long nativeBisonContents);
        String getProductVersion();
        void logCommandLineForDebugging();
    }

}
