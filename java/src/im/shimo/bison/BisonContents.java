package im.shimo.bison;

import android.content.Context;
import android.text.TextUtils;
import android.util.Base64;
import android.view.ActionMode;
import android.view.Menu;
import android.view.MenuItem;
import android.webkit.JavascriptInterface;
import android.widget.FrameLayout;

import org.chromium.base.Callback;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.task.PostTask;
import org.chromium.components.embedder_support.view.ContentView;
import org.chromium.components.embedder_support.view.ContentViewRenderView;
import org.chromium.components.navigation_interception.InterceptNavigationDelegate;
import org.chromium.components.navigation_interception.NavigationParams;
import org.chromium.content_public.browser.ActionModeCallbackHelper;
import org.chromium.content_public.browser.JavaScriptCallback;
import org.chromium.content_public.browser.JavascriptInjector;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.NavigationController;
import org.chromium.content_public.browser.SelectionPopupController;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.navigation_controller.LoadURLType;
import org.chromium.content_public.browser.navigation_controller.UserAgentOverrideOption;
import org.chromium.content_public.common.ContentUrlConstants;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.PageTransition;
import org.chromium.ui.base.WindowAndroid;

import java.lang.annotation.Annotation;
import java.util.HashMap;
import java.util.Map;

@JNINamespace("bison")
class BisonContents extends FrameLayout {
    private static final String TAG = "BisonContents";
    private final BisonWebContentsObserver mBisonWebContentsObserver;

    private long mNativeBisonContents;

    private WebContents mWebContents;
    private NavigationController mNavigationController;

    private WindowAndroid mWindow;
    private ContentViewRenderView mContentViewRenderView;
    private BisonViewAndroidDelegate mViewAndroidDelegate;


    private BisonViewClient mBisonViewClient;
    private BisonWebChromeClient mBisonWebChromeClient;

    private final BisonContentsClientBridge mBisonContentsClientBridge;
    private final BisonContentsBackgroundThreadClient mBackgroundThreadClient;
    private final BisonContentsIoThreadClient mIoThreadClient;
    private final InterceptNavigationDelegateImpl mInterceptNavigationDelegate;

    private BisonSettings mSettings;


    private JavascriptInjector mJavascriptInjector;
    private BisonContentsClient mContentsClient;

    public BisonContents(Context context, BisonWebContentsDelegate webContentsDelegate,
                         BisonContentsClientBridge bisonContentsClientBridge,
                         BisonContentsClient bisonContentsClient) {
        super(context);
        mContentsClient = bisonContentsClient;
        mNativeBisonContents = BisonContentsJni.get().init(this);
        mWebContents = BisonContentsJni.get().getWebContents(mNativeBisonContents);

        mWindow = new ActivityWindowAndroid(context, true);
        mContentViewRenderView = new ContentViewRenderView(getContext());
        mContentViewRenderView.onNativeLibraryLoaded(mWindow);
        mWindow.setAnimationPlaceholderView(mContentViewRenderView.getSurfaceView());

        addView(mContentViewRenderView);

        ContentView cv = ContentView.createContentView(context, mWebContents);
        mViewAndroidDelegate = new BisonViewAndroidDelegate(cv);
        //if (mWebContents != null) mWebContents.clearNativeReference();
        mWebContents.initialize(
                "", mViewAndroidDelegate, cv, mWindow, WebContents.createDefaultInternalsHolder());
        SelectionPopupController.fromWebContents(mWebContents)
                .setActionModeCallback(defaultActionCallback());
        mBisonWebContentsObserver = new BisonWebContentsObserver(mWebContents, this,
                bisonContentsClient);
        mNavigationController = mWebContents.getNavigationController();
        if (getParent() != null) mWebContents.onShow();
        addView(cv);
        cv.requestFocus();
        mContentViewRenderView.setCurrentWebContents(mWebContents);

        mSettings = new BisonSettings(context);

        mBisonContentsClientBridge = bisonContentsClientBridge;
        mBackgroundThreadClient = new BackgroundThreadClientImpl();
        mIoThreadClient = new IoThreadClientImpl();
        mInterceptNavigationDelegate = new InterceptNavigationDelegateImpl();


        BisonContentsJni.get().setJavaPeers(mNativeBisonContents, webContentsDelegate,
                mBisonContentsClientBridge, mIoThreadClient, mInterceptNavigationDelegate);
        mSettings.setWebContents(mWebContents);



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
            BisonContentsJni.get().grantFileSchemeAccesstoChildProcess(
                    mNativeBisonContents);
        }

        if (params.getUrl() != null && params.getUrl().equals(mWebContents.getLastCommittedUrl())
                && params.getTransitionType() == PageTransition.TYPED) {
            params.setTransitionType(PageTransition.RELOAD);
        }
        params.setOverrideUserAgent(UserAgentOverrideOption.TRUE);

        params.setTransitionType(
                params.getTransitionType() | PageTransition.FROM_API);

        mNavigationController.loadUrl(params);
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

    private JavascriptInjector getJavascriptInjector() {
        if (mJavascriptInjector == null) {
            mJavascriptInjector = JavascriptInjector.fromWebContents(mWebContents);
        }
        return mJavascriptInjector;
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
        // All methods are called on the background thread.

    }

    private class InterceptNavigationDelegateImpl implements InterceptNavigationDelegate {
        @Override
        public boolean shouldIgnoreNavigation(NavigationParams navigationParams) {
            if (!navigationParams.isRendererInitiated){
                mContentsClient.onPageStarted(navigationParams.url);
            }
            
            return false;
        }
    }


    @NativeMethods
    interface Natives {
        long init(BisonContents caller);

        WebContents getWebContents(long nativeBisonContents);

        void setJavaPeers(long nativeBisonContents, BisonWebContentsDelegate webContentsDelegate,
                          BisonContentsClientBridge bisonContentsClientBridge,
                          BisonContentsIoThreadClient ioThreadClient,
                          InterceptNavigationDelegate interceptNavigationDelegate);

        void grantFileSchemeAccesstoChildProcess(long nativeBisonContents);

        void destroy(long nativeBisonContents);
    }

}