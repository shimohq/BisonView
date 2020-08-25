package im.shimo.bison;

import android.app.Activity;
import android.content.Context;
import android.graphics.drawable.ClipDrawable;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Base64;
import android.view.ActionMode;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;
import android.webkit.ValueCallback;

import org.chromium.base.Log;
import org.chromium.base.Callback;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.task.PostTask;
import org.chromium.components.embedder_support.view.ContentView;
import org.chromium.components.embedder_support.view.ContentViewRenderView;
import org.chromium.content_public.browser.ActionModeCallbackHelper;
import org.chromium.content_public.browser.JavaScriptCallback;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.NavigationController;
import org.chromium.content_public.browser.SelectionPopupController;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.content_public.common.ContentUrlConstants;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.ViewAndroidDelegate;
import org.chromium.ui.base.WindowAndroid;

import java.util.HashMap;
import java.util.Map;

@JNINamespace("bison")
class BisonContents extends FrameLayout {
    private static final String TAG = "BisonContents";

    private long mNativeBisonContents;

    private WebContents mWebContents;
    private NavigationController mNavigationController;

    private WindowAndroid mWindow;
    private ContentViewRenderView mContentViewRenderView;
    private BisonViewAndroidDelegate mViewAndroidDelegate;


    private BisonViewClient mBisonViewClient;

    public BisonContents(Context context) {
        super(context);
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
        mNavigationController = mWebContents.getNavigationController();
        if (getParent() != null) mWebContents.onShow();
       
        addView(cv);
        cv.requestFocus();
        mContentViewRenderView.setCurrentWebContents(mWebContents);
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


    public void loadUrl(String url){
        if (url == null) return;

        if (TextUtils.equals(url, mWebContents.getLastCommittedUrl())) {
            mNavigationController.reload(true);
        } else {
            mNavigationController.loadUrl(new LoadUrlParams(sanitizeUrl(url)));
        }
        //mUrlTextView.clearFocus();
        // TODO(aurimas): Remove this when crbug.com/174541 is fixed.
        // getContentView().clearFocus();
        // getContentView().requestFocus();
    }

    public void postUrl(String url, byte[] postData) {
        LoadUrlParams params = LoadUrlParams.createLoadHttpPostParams(url, postData);
        Map<String, String> headers = new HashMap<String, String>();
        headers.put("Content-Type", "application/x-www-form-urlencoded");
        params.setExtraHeaders(headers);
        loadUrl(params);
    }

    private void loadUrl(LoadUrlParams params) {
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
        mWebContents.evaluateJavaScript(script,jsCallback);
    }

    public void setBisonViewClient(BisonViewClient client) {
        this.mBisonViewClient = client;
    }

    
    public WebContents getWebContents() {
        return mWebContents;
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
        // mWindow = null;
        // mNativeBisonView = 0;
        // mWebContents = null;
    }


    public static String sanitizeUrl(String url) {
        if (url == null) return null;
        if (url.startsWith("www.") || url.indexOf(":") == -1) url = "http://" + url;
        return url;
    }


    @SuppressWarnings("unused")
    @CalledByNative
    private void onUpdateUrl(String url) {
        //mUrlTextView.setText(url);
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void onLoadProgressChanged(double progress) {
        //removeCallbacks(mClearProgressRunnable);
        // mProgressDrawable.setLevel((int) (10000.0 * progress));
        // if (progress == 1.0) postDelayed(mClearProgressRunnable, COMPLETED_PROGRESS_TIMEOUT_MS);
    }

    @CalledByNative
    private void toggleFullscreenModeForTab(boolean enterFullscreen) {
        // mIsFullscreen = enterFullscreen;
        // LinearLayout toolBar = (LinearLayout) findViewById(R.id.toolbar);
        // toolBar.setVisibility(enterFullscreen ? GONE : VISIBLE);
    }

    @CalledByNative
    private boolean isFullscreenForTabOrPending() {
        return false;
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void setIsLoading(boolean loading) {
        // mLoading = loading;
        // if (mLoading) {
        //     mStopReloadButton
        //             .setImageResource(android.R.drawable.ic_menu_close_clear_cancel);
        // } else {
        //     //mStopReloadButton.setImageResource(R.drawable.ic_refresh);
        // }
    }


    /**
     * Initializes the ContentView based on the native tab contents pointer passed in.
     * @param webContents A {@link WebContents} object.
     */
    @SuppressWarnings("unused")
    @CalledByNative
    private void initFromNativeTabContents(WebContents webContents) {
        // Context context = getContext();
        // ContentView cv = ContentView.createContentView(context, webContents);
        // mViewAndroidDelegate = new BisonViewAndroidDelegate(cv);
        // assert (mWebContents != webContents);
        // if (mWebContents != null) mWebContents.clearNativeReference();
        // webContents.initialize(
        //         "", mViewAndroidDelegate, cv, mWindow, WebContents.createDefaultInternalsHolder());
        // mWebContents = webContents;
        // SelectionPopupController.fromWebContents(webContents)
        //         .setActionModeCallback(defaultActionCallback());
        // mNavigationController = mWebContents.getNavigationController();
        // if (getParent() != null) mWebContents.onShow();
        // if (mWebContents.getVisibleUrl() != null) {
        //     //mUrlTextView.setText(mWebContents.getVisibleUrl());
        // }
        // // ((FrameLayout) findViewById(R.id.contentview_holder)).addView(cv,
        // //         new FrameLayout.LayoutParams(
        // //                 FrameLayout.LayoutParams.MATCH_PARENT,
        // //                 FrameLayout.LayoutParams.MATCH_PARENT));
        // addView(cv);
        // cv.requestFocus();
        // mContentViewRenderView.setCurrentWebContents(mWebContents);
    }


    @CalledByNative
    public void setOverlayMode(boolean useOverlayMode) {
        // mContentViewRenderView.setOverlayVideoMode(useOverlayMode);
        // if (mOverlayModeChangedCallbackForTesting != null) {
        //     mOverlayModeChangedCallbackForTesting.onResult(useOverlayMode);
        // }
    }

    @CalledByNative
    public void sizeTo(int width, int height) {
        // mWebContents.setSize(width, height);
    }

    
    /**
     * Enable/Disable navigation(Prev/Next) button if navigation is allowed/disallowed
     * in respective direction.
     * @param controlId Id of button to update
     * @param enabled enable/disable value
     */
    @CalledByNative
    private void enableUiControl(int controlId, boolean enabled) {
        // if (controlId == 0) {
        //     mPrevButton.setEnabled(enabled);
        // } else if (controlId == 1) {
        //     mNextButton.setEnabled(enabled);
        // }
    }


    @NativeMethods

    interface Natives {
        long init(BisonContents caller);
        WebContents getWebContents(long nativeBisonContents);
        void closeShell(long BisonViewPtr);
    }

}