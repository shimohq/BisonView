package im.shimo.bison;

import android.content.Context;
import android.content.res.Configuration;
import android.os.Message;
import android.text.TextUtils;
import android.util.Base64;
import android.view.ActionMode;
import android.view.Menu;
import android.view.MenuItem;
import android.webkit.JavascriptInterface;
import android.widget.FrameLayout;
import android.view.ViewGroup;

import org.chromium.base.Callback;
import org.chromium.base.Log;
import org.chromium.base.LocaleUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.task.AsyncTask;
import org.chromium.base.task.PostTask;
import org.chromium.components.autofill.AutofillProvider;
import org.chromium.components.navigation_interception.InterceptNavigationDelegate;
import org.chromium.components.navigation_interception.NavigationParams;
import org.chromium.content_public.browser.ActionModeCallbackHelper;
import org.chromium.content_public.browser.ImeAdapter;
import org.chromium.content_public.browser.ImeEventObserver;
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
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.PageTransition;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.display.DisplayAndroid.DisplayAndroidObserver;
import org.chromium.network.mojom.ReferrerPolicy;

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

@JNINamespace("bison")
class BisonContents extends FrameLayout {
    private static final String TAG = "BisonContents";
    private static final boolean TRACE = true;
    private static final String PRODUCT_VERSION = BisonContentsJni.get().getProductVersion();
    private static final String WEB_ARCHIVE_EXTENSION = ".mht";

    private static String sCurrentLocales = "";

    private final BisonWebContentsObserver mBisonWebContentsObserver;

    private long mNativeBisonContents;

    private WebContents mWebContents;
    private NavigationController mNavigationController;

    private final BisonLayoutSizer mLayoutSizer;

    private WindowAndroid mWindow;
    private ContentViewRenderView mContentViewRenderView;
    private ContentView mContentView;
    private BisonViewAndroidDelegate mViewAndroidDelegate;


    private BisonViewClient mBisonViewClient;
    private BisonWebChromeClient mBisonWebChromeClient;

    private final BisonContentsClientBridge mBisonContentsClientBridge;
    private final BisonContentsBackgroundThreadClient mBackgroundThreadClient;
    private final BisonContentsIoThreadClient mIoThreadClient;
    private final InterceptNavigationDelegateImpl mInterceptNavigationDelegate;

    private BisonAutofillClient mAutofillClient;

    private final DisplayAndroidObserver mDisplayObserver;
    private BisonSettings mSettings;

    private AutofillProvider mAutofillProvider;
    private WebContentsInternals mWebContentsInternals;

    private JavascriptInjector mJavascriptInjector;
    private BisonContentsClient mContentsClient;
    private BisonBrowserContext mBrowserContext;

    private boolean mIsContentVisible;
    private boolean mIsUpdateVisibilityTaskPending ;
    private Runnable mUpdateVisibilityRunnable;





















    
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
            if (!navigationParams.isRendererInitiated){
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
    public BisonContents(Context context, BisonBrowserContext bisonBrowserContext,  BisonWebContentsDelegate webContentsDelegate,
                         BisonContentsClientBridge bisonContentsClientBridge,
                         BisonContentsClient bisonContentsClient) {
        super(context);
        mBrowserContext = bisonBrowserContext;
        mContentsClient = bisonContentsClient;
        mNativeBisonContents = BisonContentsJni.get().init(this,mBrowserContext.getNativePointer());
        mWebContents = BisonContentsJni.get().getWebContents(mNativeBisonContents);

        mWindow = new ActivityWindowAndroid(context, true);
        mContentViewRenderView = new ContentViewRenderView(getContext());
        mContentViewRenderView.onNativeLibraryLoaded(mWindow);
        mWindow.setAnimationPlaceholderView(mContentViewRenderView.getSurfaceView());

        addView(mContentViewRenderView);

        mContentView = ContentView.createContentView(context, mWebContents);
        mViewAndroidDelegate = new BisonViewAndroidDelegate(mContentView);
        //webContentsDelegate.setContainerView(mContentView);
        
        mWebContents.initialize(
                PRODUCT_VERSION, mViewAndroidDelegate, mContentView, mWindow, WebContents.createDefaultInternalsHolder());
        SelectionPopupController.fromWebContents(mWebContents)
                .setActionModeCallback(defaultActionCallback());
        mBisonWebContentsObserver = new BisonWebContentsObserver(mWebContents, this,
                bisonContentsClient);
        mNavigationController = mWebContents.getNavigationController();
        if (getParent() != null) mWebContents.onShow();
        addView(mContentView);
        mContentView.requestFocus();
        mContentViewRenderView.setCurrentWebContents(mWebContents);

        mSettings = new BisonSettings(context);

        mBisonContentsClientBridge = bisonContentsClientBridge;
        mBackgroundThreadClient = new BackgroundThreadClientImpl();
        mIoThreadClient = new IoThreadClientImpl();
        mInterceptNavigationDelegate = new InterceptNavigationDelegateImpl();

        mLayoutSizer = new BisonLayoutSizer();
        mLayoutSizer.setDelegate(new LayoutSizerDelegate());
        mDisplayObserver = new BisonDisplayAndroidObserver();

        BisonContentsJni.get().setJavaPeers(mNativeBisonContents, webContentsDelegate,
                mBisonContentsClientBridge, mIoThreadClient, mInterceptNavigationDelegate);
        mSettings.setWebContents(mWebContents);

        mDisplayObserver.onDIPScaleChanged(getDeviceScaleFactor());

        mUpdateVisibilityRunnable = () -> updateWebContentsVisibility();

        updateDefaultLocale();

        
    }
    private void initWebContents(ViewAndroidDelegate viewDelegate,
            InternalAccessDelegate internalDispatcher, WebContents webContents,
            WindowAndroid windowAndroid, WebContentsInternalsHolder internalsHolder) {
        webContents.initialize(
                PRODUCT_VERSION, viewDelegate, internalDispatcher, windowAndroid, internalsHolder);
        mViewEventSink = ViewEventSink.from(mWebContents);
        mViewEventSink.setHideKeyboardOnBlur(false);
        SelectionPopupController controller = SelectionPopupController.fromWebContents(webContents);
        controller.setActionModeCallback(new AwActionModeCallback(mContext, this, webContents));
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


    private JavascriptInjector getJavascriptInjector() {
        if (mJavascriptInjector == null) {
            mJavascriptInjector = JavascriptInjector.fromWebContents(mWebContents);
        }
        return mJavascriptInjector;
    }

     @CalledByNative
    private void onRendererResponsive(BisonRenderProcess renderProcess) {
        //if (isDestroyed(NO_WARN)) return;
        mContentsClient.onRendererResponsive(renderProcess);
    }

    @CalledByNative
    private void onRendererUnresponsive(BisonRenderProcess renderProcess) {
        //if (isDestroyed(NO_WARN)) return;
        mContentsClient.onRendererUnresponsive(renderProcess);
    }

    /**
     * {link @ActionMode.Callback} that uses the default implementation in
     * {@link SelectionPopupController}.
     */
    private ActionMode.Callback defaultActionCallback() {
        final ActionModeCallbackHelper helper =
                SelectionPopupController.fromWebContents(mWebContents)
                        .getActionModeCallbackHelper();

        return new ActionMode.Callback() {
            @Override
            public boolean onCreateActionMode(ActionMode mode, Menu menu) {
                helper.onCreateActionMode(mode, menu);
                return true;
            }

            @Override
            public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
                return helper.onPrepareActionMode(mode, menu);
            }

            @Override
            public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
                return helper.onActionItemClicked(mode, item);
            }

            @Override
            public void onDestroyActionMode(ActionMode mode) {
                helper.onDestroyActionMode();
            }
        };
    }


    public void loadUrl(String url) {
        if (url == null) return;
        loadUrl(url, null);
    }

    public void loadUrl(String url, Map<String, String> additionalHttpHeaders) {
        if (url == null) {
            return;
        }
        LoadUrlParams params = new LoadUrlParams(url, PageTransition.TYPED);
        // LoadUrlParams params = new LoadUrlParams(url);
        if (additionalHttpHeaders != null) {
            params.setExtraHeaders(new HashMap<>(additionalHttpHeaders));
        }

        loadUrl(params);
    }

    public void postUrl(String url, byte[] postData) {
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

    public void evaluateJavaScript(String script, Callback<String> callback) {
        JavaScriptCallback jsCallback = null;
        if (callback != null) {
            jsCallback = jsonResult -> {
                PostTask.postTask(UiThreadTaskTraits.DEFAULT, () -> callback.onResult(jsonResult));
            };
        }
        mWebContents.evaluateJavaScript(script, jsCallback);
    }

    public void setBisonViewClient(BisonViewClient client) {
        this.mBisonViewClient = client;
    }

    public void setBisonWebChromeClient(BisonWebChromeClient client) {
        this.mBisonWebChromeClient = client;
    }

    public WebContents getWebContents() {
        return mWebContents;
    }


    public void stopLoading() {
        mWebContents.stop();
    }

    public void reload() {
        mNavigationController.reload(true);
    }

    public boolean canGoBack() {
        return mNavigationController.canGoBack();
    }

    public void goBack() {
        mNavigationController.goBack();
    }

    public boolean canGoForward() {
        return mNavigationController.canGoForward();
    }

    public void goForward() {
        mNavigationController.goForward();
    }

    public boolean canGoBackOrForward(int steps) {
        return mNavigationController.canGoToOffset(steps);
    }

    public void goBackOrForward(int steps) {
        mNavigationController.goToOffset(steps);
    }

    public void documentHasImages(Message message) {
        // if (!isDestroyed(WARN)) {}
        BisonContentsJni.get().documentHasImages(mNativeBisonContents, message);
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
        //if (isDestroyed(WARN)) return null;
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
        //isDestroyed(WARN) ? null :
        return mNavigationController.getNavigationHistory();
    }
    public String getTitle() {
        return mWebContents.getTitle();
    }

    public String getUrl() {
        String url = mWebContents.getVisibleUrl();
        if (url == null || url.trim().isEmpty()) return null;
        return url;
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
        mWindow = null;
        mNativeBisonContents = 0;
        mWebContents = null;
    }


    public static String sanitizeUrl(String url) {
        if (url == null) return null;
        if (url.startsWith("www.") || url.indexOf(":") == -1) url = "http://" + url;
        return url;
    }

    public void addJavascriptInterface(Object object, String name) {
        //if (TRACE) Log.i(TAG, "%s addJavascriptInterface=%s", this, name);
        //if (isDestroyed(WARN)) return;
        Class<? extends Annotation> requiredAnnotation = JavascriptInterface.class;
        getJavascriptInjector().addPossiblyUnsafeInterface(object, name, requiredAnnotation);
    }

    public void disableJavascriptInterfacesInspection() {
//        if (!isDestroyed(WARN)) {
//            getJavascriptInjector().setAllowInspection(false);
//        }
        getJavascriptInjector().setAllowInspection(false);
    }

    


    public BisonSettings getSettings() {
        return mSettings;
    }

    public void destroy() {
        if (mContentViewRenderView != null) {
            mContentViewRenderView.destroy();
        }
        removeAllViews();
        BisonContentsJni.get().destroy(mNativeBisonContents);
    }


    private float getDeviceScaleFactor() {
        return mWindow.getDisplay().getDipScale();
    }


    

    

    @CalledByNative
    private static void onDocumentHasImagesResponse(boolean result, Message message) {
        message.arg1 = result ? 1 : 0;
        message.sendToTarget();
    }

    /** Callback for generateMHTML. */
    @CalledByNative
    private static void generateMHTMLCallback(String path, long size, Callback<String> callback) {
        if (callback == null) return;
        callback.onResult(size < 0 ? null : path);
    }


    public BisonGeolocationPermissions getGeolocationPermissions() {
        return mBrowserContext.getGeolocationPermissions();
    }

    public void invokeGeolocationCallback(boolean value, String requestingFrame) {
        BisonContentsJni.get().invokeGeolocationCallback(
                mNativeBisonContents, value, requestingFrame);
    }

    @CalledByNative
    private void onGeolocationPermissionsShowPrompt(String origin) {
        // if (isDestroyed(NO_WARN)) return;
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
        //|| isDestroyed(WARN)
        if (path == null ) {
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

    
    

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
    }    

     @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
    }

    @Override
    public void onWindowFocusChanged(boolean hasWindowFocus) {
        super.onDetachedFromWindow();
    }

    





    @CalledByNative
    private void onPermissionRequestCanceled(BisonPermissionRequest permissionRequest) {
        mContentsClient.onPermissionRequestCanceled(permissionRequest);
    }

    // Return true if the GeolocationPermissionAPI should be used.
    @CalledByNative
    private boolean useLegacyGeolocationPermissionAPI() {
        // Always return true since we are not ready to swap the geolocation yet.
        // TODO: If we decide not to migrate the geolocation, there are some unreachable
        // code need to remove. http://crbug.com/396184.
        return true;
    }

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

    

    private void postUpdateWebContentsVisibility() {
        if (mIsUpdateVisibilityTaskPending) return;
        mIsUpdateVisibilityTaskPending = true;
        PostTask.postTask(UiThreadTaskTraits.DEFAULT, mUpdateVisibilityRunnable);
    }

    private void updateWebContentsVisibility() {
        mIsUpdateVisibilityTaskPending = false;
        // if (isDestroyed(NO_WARN)) return;
        //boolean contentVisible = BisonContentsJni.get().isVisible(mNativeBisonContents, this);

        // if (!mIsContentVisible) {
        //     mWebContents.onShow();
        // } else if (!contentVisible && mIsContentVisible) {
        //     mWebContents.onHide();
        // }
        mWebContents.onShow();
        // mIsContentVisible = contentVisible;
        //updateChildProcessImportance();
    }

    @NativeMethods
    interface Natives {
        long init(BisonContents caller,long nativeBisonBrowserContext);

        void destroy(long nativeBisonContents);
        WebContents getWebContents(long nativeBisonContents);
        void updateDefaultLocale(String locale, String localeList);
        void setJavaPeers(long nativeBisonContents, BisonWebContentsDelegate webContentsDelegate,
                          BisonContentsClientBridge bisonContentsClientBridge,
                          BisonContentsIoThreadClient ioThreadClient,
                          InterceptNavigationDelegate interceptNavigationDelegate);

        void documentHasImages(long nativeBisonContents, Message message);
        void generateMHTML(
                long nativeBisonContents, String path, Callback<String> callback);

        void setDipScale(long nativeBisonContents, BisonContents caller, float dipScale);
        
        void grantFileSchemeAccesstoChildProcess(long nativeBisonContents);

        void setExtraHeadersForUrl(
                long nativeBisonContents, String url, String extraHeaders);
        void invokeGeolocationCallback(
                long nativeBisonContents, boolean value, String requestingFrame);

        String getProductVersion();
    }

}