package im.shimo.bison.adapter;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Picture;
import android.net.Uri;
import android.net.http.SslError;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;

import im.shimo.bison.BisonView;
import im.shimo.bison.BisonWebChromeClient;
import im.shimo.bison.ClientCertRequest;
import im.shimo.bison.ConsoleMessage;
import im.shimo.bison.HttpAuthHandler;
import im.shimo.bison.JsPromptResult;
import im.shimo.bison.JsResult;
import im.shimo.bison.PermissionRequest;
import im.shimo.bison.SslErrorHandler;
import im.shimo.bison.ValueCallback;
import im.shimo.bison.WebResourceResponse;
import im.shimo.bison.internal.BvConsoleMessage;
import im.shimo.bison.internal.BvContentsClient;
import im.shimo.bison.internal.BvContentsClientBridge;
import im.shimo.bison.internal.BvGeolocationPermissions;
import im.shimo.bison.internal.BvHttpAuthHandler;
import im.shimo.bison.internal.BvPermissionRequest;
import im.shimo.bison.internal.BvWebResourceRequest;
import im.shimo.bison.internal.BvWebResourceResponse;
import im.shimo.bison.internal.JsDialogHelper;
import im.shimo.bison.internal.JsPromptResultReceiver;
import im.shimo.bison.internal.JsResultReceiver;
import im.shimo.bison.internal.PermissionResource;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.TraceEvent;

import java.lang.ref.WeakReference;
import java.security.Principal;
import java.security.PrivateKey;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.WeakHashMap;

import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public class BisonViewContentsClientAdapter extends BvContentsClientAdapter {

    private BisonWebChromeClient mBisonWebChromeClient;
    private BisonView.FindListener mFindListener;
    private BisonView.PictureListener mPictureListener;
    private BisonView.DownloadListener mDownloadListener;

    private WeakHashMap<BvPermissionRequest, WeakReference<PermissionRequestAdapter>>
            mOngoingPermissionRequests;

    public BisonViewContentsClientAdapter(BisonView bisonView, Context context) {
        super(bisonView, context);
    }

    public void setBisonWebChromeClient(BisonWebChromeClient bisonWebChromeClient) {
        mBisonWebChromeClient = bisonWebChromeClient;
    }

    BisonWebChromeClient getBisonWebChromeClient() {
        return mBisonWebChromeClient;
    }

    public void setDownloadListener(BisonView.DownloadListener listener) {
        mDownloadListener = listener;
    }

    public void setFindListener(BisonView.FindListener listener) {
        mFindListener = listener;
    }

    /**
     * @see BvContentsClient#getVisitedHistory.
     */
    @Override
    public void getVisitedHistory(Callback<String[]> callback) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.getVisitedHistory");
            if (mBisonWebChromeClient != null) {
                if (TRACE)
                    Log.i(TAG, "getVisitedHistory");
                mBisonWebChromeClient.getVisitedHistory(callback == null ? null : value -> callback.onResult(value));
            }
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.getVisitedHistory");
        }
    }

    @Override
    public void doUpdateVisitedHistory(String url, boolean isReload) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.doUpdateVisitedHistory");
            if (TRACE)
                Log.i(TAG, "doUpdateVisitedHistory=" + url + " reload=" + isReload);
            mBisonViewClient.doUpdateVisitedHistory(mBisonView, url, isReload);
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.doUpdateVisitedHistory");
        }
    }

    /**
     * @see BvContentsClient#onProgressChanged(int)
     */
    @Override
    public void onProgressChanged(int progress) {
        if (mBisonWebChromeClient != null) {
            mBisonWebChromeClient.onProgressChanged(mBisonView, progress);
        }
    }

    /**
     * @see BvContentsClient#shouldInterceptRequest
     */
    @Override
    public BvWebResourceResponse shouldInterceptRequest(BvWebResourceRequest request) {
        WebResourceResponse response = mBisonViewClient.shouldInterceptRequest(mBisonView,
                new WebResourceRequestAdapter(request));
        if (response == null)
            return null;

        Map<String, String> responseHeaders = response.getResponseHeaders();
        if (responseHeaders == null)
            responseHeaders = new HashMap<String, String>();

        return new BvWebResourceResponse(
            response.getMimeType(),
            response.getEncoding(),
            response.getData(),
            response.getStatusCode(),
            response.getReasonPhrase(),
            responseHeaders);
    }

    @Override
    public void onUnhandledKeyEvent(KeyEvent event) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onUnhandledKeyEvent");
            if (TRACE)
                Log.i(TAG, "onUnhandledKeyEvent");
            mBisonViewClient.onUnhandledKeyEvent(mBisonView, event);
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onUnhandledKeyEvent");
        }
    }


    @Override
    public boolean onConsoleMessage(BvConsoleMessage consoleMessage) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onConsoleMessage");
            boolean result;
            if (mBisonWebChromeClient != null) {
                if (TRACE)
                    Log.i(TAG, "onConsoleMessage: " + consoleMessage.message());
                result = mBisonWebChromeClient.onConsoleMessage(fromBvConsoleMessage(consoleMessage));
            } else {
                result = false;
            }
            return result;
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onConsoleMessage");
        }
    }

    @Override
    public void onFindResultReceived(int activeMatchOrdinal, int numberOfMatches,
            boolean isDoneCounting) {
        try {
            TraceEvent.begin("BvContentsClient.onFindResultReceived");
            if (mFindListener == null)
                return;
            if (TRACE)
                Log.i(TAG, "onFindResultReceived");
            mFindListener.onFindResultReceived(activeMatchOrdinal, numberOfMatches, isDoneCounting);
        } finally {
            TraceEvent.end("BvContentsClient.onFindResultReceived");
        }
    }


    @Override
    public void onNewPicture(Picture picture) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onNewPicture");
            if (mPictureListener == null)
                return;
            if (TRACE)
                Log.i(TAG, "onNewPicture");
            mPictureListener.onNewPicture(mBisonView, picture);
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onNewPicture");
        }
    }

    @Override
    public void onLoadResource(String url) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onLoadResource");
            if (TRACE)
                Log.i(TAG, "onLoadResource=" + url);
            mBisonViewClient.onLoadResource(mBisonView, url);

            // Record UMA for onLoadResource.
            // BvHistogramRecorder.recordCallbackInvocation(
            //     BvHistogramRecorder.WebViewCallbackType.ON_LOAD_RESOURCE);
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onLoadResource");
        }
    }

    @Override
    public boolean onCreateWindow(boolean isDialog, boolean isUserGesture) {
        // jiang947
        try {
            TraceEvent.begin("BvContentsClient.onCreateWindow");
            boolean result;
            if (mBisonWebChromeClient != null) {
                if (TRACE)
                    Log.i(TAG, "onCreateWindow");
                result = mBisonWebChromeClient.onCreateWindow(mBisonView, isDialog, isUserGesture,
                        null);
            } else {
                result = false;
            }
            return result;
        } finally {
            TraceEvent.end("BvContentsClient.onCreateWindow");
        }
    }


    @Override
    public void onCloseWindow() {
        try {
            TraceEvent.begin("BvContentsClient.onCloseWindow");
            if (mBisonWebChromeClient != null) {
                if (TRACE)
                    Log.i(TAG, "onCloseWindow");
                mBisonWebChromeClient.onCloseWindow(mBisonView);
            }
        } finally {
            TraceEvent.end("BvContentsClient.onCloseWindow");
        }
    }


    @Override
    public void onRequestFocus() {
        try {
            TraceEvent.begin("BvContentsClient.onRequestFocus");
            if (mBisonWebChromeClient != null) {
                if (TRACE)
                    Log.i(TAG, "onRequestFocus");
                mBisonWebChromeClient.onRequestFocus(mBisonView);
            }
        } finally {
            TraceEvent.end("BvContentsClient.onRequestFocus");
        }
    }


    @Override
    public void onReceivedTouchIconUrl(String url, boolean precomposed) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onReceivedTouchIconUrl");
            if (mBisonWebChromeClient != null) {
                if (TRACE)
                    Log.i(TAG, "onReceivedTouchIconUrl=" + url);
                mBisonWebChromeClient.onReceivedTouchIconUrl(mBisonView, url, precomposed);
            }
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onReceivedTouchIconUrl");
        }
    }


    @Override
    public void onReceivedIcon(Bitmap bitmap) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onReceivedIcon");
            if (mBisonWebChromeClient != null) {
                if (TRACE)
                    Log.i(TAG, "onReceivedIcon");
                mBisonWebChromeClient.onReceivedIcon(mBisonView, bitmap);
            }
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onReceivedIcon");
        }
    }


    @Override
    public void onPageStarted(String url) {
        // jiang 未完成
        mBisonViewClient.onPageStarted(mBisonView, url, null);
    }


    @Override
    public void onPageFinished(String url) {
        // jiang 未完成
        mBisonViewClient.onPageFinished(mBisonView, url);
    }

    /**
     * @see ContentViewClient#onReceivedTitle(String)
     */
    @Override
    public void onReceivedTitle(String title) {
        if (mBisonWebChromeClient != null) {
            mBisonWebChromeClient.onReceivedTitle(mBisonView, title);
        }
    }

    /**
     * @see ContentViewClient#shouldOverrideKeyEvent(KeyEvent)
     */
    @Override
    public boolean shouldOverrideKeyEvent(KeyEvent event) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.shouldOverrideKeyEvent");
            if (TRACE)
                Log.i(TAG, "shouldOverrideKeyEvent");

            return mBisonViewClient.shouldOverrideKeyEvent(mBisonView, event);
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.shouldOverrideKeyEvent");
        }
    }

    @Override
    public void onGeolocationPermissionsShowPrompt(String origin,
            BvGeolocationPermissions.Callback callback) {

        TraceEvent.begin("WebViewContentsClientAdapter.onGeolocationPermissionsShowPrompt");
        if (mBisonWebChromeClient == null) {
            callback.invoke(origin, false, false);
            return;
        }

        mBisonWebChromeClient.onGeolocationPermissionsShowPrompt(origin,
                callback == null ? null
                        : (callbackOrigin, allow, retain) -> callback.invoke(callbackOrigin, allow,
                                retain));
    }

    @Override
    public void onGeolocationPermissionsHidePrompt() {
        if (mBisonWebChromeClient != null) {
            // if (TRACE) Log.i(TAG, "onGeolocationPermissionsHidePrompt");
            mBisonWebChromeClient.onGeolocationPermissionsHidePrompt();
        }
    }

    @Override
    public void onPermissionRequest(BvPermissionRequest permissionRequest) {
        try {
            TraceEvent.begin("BvContentsClient.onPermissionRequest");
            if (mBisonWebChromeClient != null) {
                // if (TRACE) Log.i(TAG, "onPermissionRequest");
                if (mOngoingPermissionRequests == null) {
                    mOngoingPermissionRequests =
                            new WeakHashMap<BvPermissionRequest, WeakReference<PermissionRequestAdapter>>();
                }
                PermissionRequestAdapter adapter = new PermissionRequestAdapter(permissionRequest);
                mOngoingPermissionRequests.put(permissionRequest,
                        new WeakReference<PermissionRequestAdapter>(adapter));
                mBisonWebChromeClient.onPermissionRequest(adapter);
            } else {
                // By default, we deny the permission.
                permissionRequest.deny();
            }
        } finally {
            TraceEvent.end("BvContentsClient.onPermissionRequest");
        }
    }

    @Override
    public void onPermissionRequestCanceled(BvPermissionRequest permissionRequest) {
        try {
            TraceEvent.begin("BvContentsClient.onPermissionRequestCanceled");
            if (mBisonWebChromeClient != null && mOngoingPermissionRequests != null) {
                // if (TRACE) Log.i(TAG, "onPermissionRequestCanceled");
                WeakReference<PermissionRequestAdapter> weakRef =
                        mOngoingPermissionRequests.get(permissionRequest);
                // We don't hold strong reference to PermissionRequestAdpater and don't expect
                // the
                // user only holds weak reference to it either, if so, user has no way to call
                // grant()/deny(), and no need to be notified the cancellation of request.
                if (weakRef != null) {
                    PermissionRequestAdapter adapter = weakRef.get();
                    if (adapter != null)
                        mBisonWebChromeClient.onPermissionRequestCanceled(adapter);
                }
            }
        } finally {
            TraceEvent.end("BvContentsClient.onPermissionRequestCanceled");
        }
    }

    private static class JsPromptResultReceiverAdapter implements JsResult.ResultReceiver {

        private JsPromptResultReceiver mPromptResultReceiver;
        private JsResultReceiver mResultReceiver;

        private final JsPromptResult mPromptResult = new JsPromptResult(this);

        public JsPromptResultReceiverAdapter(JsPromptResultReceiver receiver) {
            mPromptResultReceiver = receiver;
        }

        public JsPromptResultReceiverAdapter(JsResultReceiver receiver) {
            mResultReceiver = receiver;
        }

        public JsPromptResult getPromptResult() {
            return mPromptResult;
        }

        @Override
        public void onJsResultComplete(JsResult result) {
            if (mPromptResultReceiver != null) {
                if (mPromptResult.getResult()) {
                    mPromptResultReceiver.confirm(mPromptResult.getStringResult());
                } else {
                    mPromptResultReceiver.cancel();
                }
            } else {
                if (mPromptResult.getResult()) {
                    mResultReceiver.confirm();
                } else {
                    mResultReceiver.cancel();
                }
            }
        }
    }

    @Override
    public void handleJsAlert(String url, String message, JsResultReceiver receiver) {
        if (mBisonWebChromeClient != null) {
            final JsPromptResult res =
                    new JsPromptResultReceiverAdapter(receiver).getPromptResult();
            if (TRACE)
                Log.i(TAG, "handleJsAlert");
            if (!mBisonWebChromeClient.onJsAlert(mBisonView, url, message, res)) {
                if (!showDefaultJsDialog(res, JsDialogHelper.ALERT, null, message, url)) {
                    receiver.cancel();
                }
            }
        } else {
            receiver.cancel();
        }
    }

    @Override
    protected void handleJsBeforeUnload(String url, String message, JsResultReceiver receiver) {
        if (mBisonWebChromeClient != null) {
            final JsPromptResult res =
                    new JsPromptResultReceiverAdapter(receiver).getPromptResult();
            if (TRACE)
                Log.i(TAG, "handleJsBeforeUnload");
            if (!mBisonWebChromeClient.onJsBeforeUnload(mBisonView, url, message, res)) {
                if (!showDefaultJsDialog(res, JsDialogHelper.UNLOAD, null, message, url)) {
                    receiver.cancel();
                }
            }
        } else {
            receiver.cancel();
        }
    }

    @Override
    protected void handleJsConfirm(String url, String message, JsResultReceiver receiver) {
        if (mBisonWebChromeClient != null) {
            final JsPromptResult res =
                    new JsPromptResultReceiverAdapter(receiver).getPromptResult();
            if (!mBisonWebChromeClient.onJsConfirm(mBisonView, url, message, res)) {
                if (!showDefaultJsDialog(res, JsDialogHelper.CONFIRM, null, message, url)) {
                    receiver.cancel();
                }
            }
        } else {
            receiver.cancel();
        }
    }

    @Override
    protected void handleJsPrompt(String url, String message, String defaultValue,
            JsPromptResultReceiver receiver) {
        if (mBisonWebChromeClient != null) {
            final JsPromptResult res =
                    new JsPromptResultReceiverAdapter(receiver).getPromptResult();
            if (TRACE)
                Log.i(TAG, "onJsPrompt");
            if (!mBisonWebChromeClient.onJsPrompt(mBisonView, url, message, defaultValue, res)) {
                if (!showDefaultJsDialog(res, JsDialogHelper.PROMPT, defaultValue, message, url)) {
                    receiver.cancel();
                }
            }
        } else {
            receiver.cancel();
        }
    }

    /**
     * Try to show the default JS dialog and return whether the dialog was shown.
     */
    private boolean showDefaultJsDialog(JsPromptResult res, int jsDialogType, String defaultValue,
            String message, String url) {
        // Note we must unwrap the Context here due to JsDialogHelper only using
        // instanceof to
        // check if a Context is an Activity.
        Context activityContext = ContextUtils.activityFromContext(mContext);
        if (activityContext == null) {
            Log.w(TAG, "Unable to create JsDialog without an Activity");
            return false;
        }
        try {
            new JsDialogHelper(res, jsDialogType, defaultValue, message, url)
                    .showDialog(activityContext);
        } catch (WindowManager.BadTokenException e) {
            Log.w(TAG,
                    "Unable to create JsDialog. Has this BisonView outlived the Activity it was created with?");
            return false;
        }
        return true;
    }


    @Override
    public void onReceivedHttpAuthRequest(BvHttpAuthHandler handler, String host, String realm) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onReceivedHttpAuthRequest");
            if (TRACE)
                Log.i(TAG, "onReceivedHttpAuthRequest=" + host);
            mBisonViewClient.onReceivedHttpAuthRequest(mBisonView,
                    new BvHttpAuthHandlerAdapter(handler), host, realm);
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onReceivedHttpAuthRequest");
        }
    }

    @SuppressWarnings("HandlerLeak")
    @Override
    public void onReceivedSslError(final Callback<Boolean> callback, SslError error) {
        try {
            TraceEvent.begin("BvContentsClient.onReceivedSslError");
            SslErrorHandler handler = new SslErrorHandler() {
                @Override
                public void proceed() {
                    callback.onResult(true);
                }

                @Override
                public void cancel() {
                    callback.onResult(false);
                }
            };
            if (TRACE)
                Log.i(TAG, "onReceivedSslError");
            mBisonViewClient.onReceivedSslError(mBisonView, handler, error);
        } finally {
            TraceEvent.end("BvContentsClient.onReceivedSslError");
        }
    }

    private static class ClientCertRequestImpl extends ClientCertRequest {
        private final BvContentsClientBridge.ClientCertificateRequestCallback mCallback;
        private final String[] mKeyTypes;
        private final Principal[] mPrincipals;
        private final String mHost;
        private final int mPort;

        public ClientCertRequestImpl(
                BvContentsClientBridge.ClientCertificateRequestCallback callback,
                String[] keyTypes, Principal[] principals, String host, int port) {
            mCallback = callback;
            mKeyTypes = keyTypes;
            mPrincipals = principals;
            mHost = host;
            mPort = port;
        }

        @Override
        public String[] getKeyTypes() {
            // This is already a copy of native argument, so return directly.
            return mKeyTypes;
        }

        @Override
        public Principal[] getPrincipals() {
            // This is already a copy of native argument, so return directly.
            return mPrincipals;
        }

        @Override
        public String getHost() {
            return mHost;
        }

        @Override
        public int getPort() {
            return mPort;
        }

        @Override
        public void proceed(final PrivateKey privateKey, final X509Certificate[] chain) {
            mCallback.proceed(privateKey, chain);
        }

        @Override
        public void ignore() {
            mCallback.ignore();
        }

        @Override
        public void cancel() {
            mCallback.cancel();
        }
    }

    @Override
    public void onReceivedClientCertRequest(
            BvContentsClientBridge.ClientCertificateRequestCallback callback, String[] keyTypes,
            Principal[] principals, String host, int port) {
        Log.i(TAG, "onReceivedClientCertRequest");
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onReceivedClientCertRequest");
            final ClientCertRequestImpl request =
                    new ClientCertRequestImpl(callback, keyTypes, principals, host, port);
            mBisonViewClient.onReceivedClientCertRequest(mBisonView, request);
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onReceivedClientCertRequest");
        }
    }

    @Override
    public void onReceivedLoginRequest(String realm, String account, String args) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onReceivedLoginRequest");
            if (TRACE)
                Log.i(TAG, "onReceivedLoginRequest=" + realm);
            mBisonViewClient.onReceivedLoginRequest(mBisonView, realm, account, args);
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onReceivedLoginRequest");
        }
    }

    @Override
    public void onFormResubmission(Message dontResend, Message resend) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onFormResubmission");
            if (TRACE)
                Log.i(TAG, "onFormResubmission");
            mBisonViewClient.onFormResubmission(mBisonView, dontResend, resend);
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onFormResubmission");
        }
    }

    @Override
    public void onDownloadStart(String url, String userAgent, String contentDisposition,
            String mimeType, long contentLength) {
        if (mDownloadListener != null) {
            mDownloadListener.onDownloadStart(url, userAgent, contentDisposition, mimeType,
                    contentLength);
        }
    }

    @Override
    public void showFileChooser(Callback<String[]> uploadFileCallback,
            BvContentsClient.FileChooserParamsImpl fileChooserParams) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.showFileChooser");
            if (mBisonWebChromeClient == null) {
                uploadFileCallback.onResult(null);
                return;
            }
            if (TRACE) Log.i(TAG, "showFileChooser");
            ValueCallback<Uri[]> callbackAdapter = new ValueCallback<Uri[]>() {
                private boolean mCompleted;
                @Override
                public void onReceiveValue(Uri[] uriList) {
                    if (mCompleted) {
                        throw new IllegalStateException(
                                "showFileChooser result was already called");
                    }
                    mCompleted = true;
                    String s[] = null;
                    if (uriList != null) {
                        s = new String[uriList.length];
                        for (int i = 0; i < uriList.length; i++) {
                            s[i] = uriList[i].toString();
                        }
                    }
                    uploadFileCallback.onResult(s);
                }
            };

            // Invoke the new callback introduced in Lollipop. If the app handles
            // it, we're done here.
            if (mBisonWebChromeClient.onShowFileChooser(
                    mBisonView, callbackAdapter, fromBvFileChooserParams(fileChooserParams))) {
                return;
            }

        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.showFileChooser");
        }
    }

    @Override
    public void onScaleChangedScaled(float oldScale, float newScale) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onScaleChangedScaled");
            if (TRACE)
                Log.i(TAG, " onScaleChangedScaled");
            mBisonViewClient.onScaleChanged(mBisonView, oldScale, newScale);
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onScaleChangedScaled");
        }
    }

    @Override
    public void onShowCustomView(View view, final CustomViewCallback cb) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onShowCustomView");
            if (mBisonWebChromeClient != null) {
                if (TRACE)
                    Log.i(TAG, "onShowCustomView");
                mBisonWebChromeClient.onShowCustomView(view,
                        cb == null ? null : () -> cb.onCustomViewHidden());
            }
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onShowCustomView");
        }
    }

    @Override
    public void onHideCustomView() {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onHideCustomView");
            if (mBisonWebChromeClient != null) {
                if (TRACE)
                    Log.i(TAG, "onHideCustomView");
                mBisonWebChromeClient.onHideCustomView();
            }
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onHideCustomView");
        }
    }

    @Override
    protected View getVideoLoadingProgressView() {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.getVideoLoadingProgressView");
            View result;
            if (mBisonWebChromeClient != null) {
                if (TRACE)
                    Log.i(TAG, "getVideoLoadingProgressView");
                result = mBisonWebChromeClient.getVideoLoadingProgressView();
            } else {
                result = null;
            }
            return result;
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.getVideoLoadingProgressView");
        }
    }

    @Override
    public Bitmap getDefaultVideoPoster() {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.getDefaultVideoPoster");
            Bitmap result = null;
            if (mBisonWebChromeClient != null) {
                if (TRACE)
                    Log.i(TAG, "getDefaultVideoPoster");
                result = mBisonWebChromeClient.getDefaultVideoPoster();
            }
            // jiang
            // if (result == null) {
            // // The ic_play_circle_outline_black_48dp icon is transparent so we need to draw it
            // // on a gray background.
            // Bitmap poster = BitmapFactory.decodeResource(mContext.getResources(),
            // org.chromium.android_webview.R.drawable.ic_play_circle_outline_black_48dp);
            // result = Bitmap.createBitmap(poster.getWidth(), poster.getHeight(),
            // poster.getConfig());
            // result.eraseColor(Color.GRAY);
            // Canvas canvas = new Canvas(result);
            // canvas.drawBitmap(poster, 0f, 0f, null);
            // }
            return result;
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.getDefaultVideoPoster");
        }
    }

    private static class BvHttpAuthHandlerAdapter extends HttpAuthHandler {
        private BvHttpAuthHandler mBisonHandler;

        public BvHttpAuthHandlerAdapter(BvHttpAuthHandler bisonHandler) {
            mBisonHandler = bisonHandler;
        }

        @Override
        public void proceed(String username, String password) {
            if (username == null) {
                username = "";
            }

            if (password == null) {
                password = "";
            }
            mBisonHandler.proceed(username, password);
        }

        @Override
        public void cancel() {
            mBisonHandler.cancel();
        }

        @Override
        public boolean useHttpAuthUsernamePassword() {
            return mBisonHandler.isFirstAttempt();
        }
    }

    /**
     * Type adaptation class for PermissionRequest.
     */
    public static class PermissionRequestAdapter extends PermissionRequest {

        private static long toBvPermissionResources(String[] resources) {
            long result = 0;
            for (String resource : resources) {
                if (resource.equals(PermissionRequest.RESOURCE_VIDEO_CAPTURE)) {
                    result |= PermissionResource.VIDEO_CAPTURE;
                } else if (resource.equals(PermissionRequest.RESOURCE_AUDIO_CAPTURE)) {
                    result |= PermissionResource.AUDIO_CAPTURE;
                } else if (resource.equals(PermissionRequest.RESOURCE_PROTECTED_MEDIA_ID)) {
                    result |= PermissionResource.PROTECTED_MEDIA_ID;
                } else if (resource.equals(BvPermissionRequest.RESOURCE_MIDI_SYSEX)) {
                    result |= PermissionResource.MIDI_SYSEX;
                }
            }
            return result;
        }

        private static String[] toPermissionResources(long resources) {
            ArrayList<String> result = new ArrayList<String>();
            if ((resources & PermissionResource.VIDEO_CAPTURE) != 0) {
                result.add(PermissionRequest.RESOURCE_VIDEO_CAPTURE);
            }
            if ((resources & PermissionResource.AUDIO_CAPTURE) != 0) {
                result.add(PermissionRequest.RESOURCE_AUDIO_CAPTURE);
            }
            if ((resources & PermissionResource.PROTECTED_MEDIA_ID) != 0) {
                result.add(PermissionRequest.RESOURCE_PROTECTED_MEDIA_ID);
            }
            if ((resources & PermissionResource.MIDI_SYSEX) != 0) {
                result.add(BvPermissionRequest.RESOURCE_MIDI_SYSEX);
            }
            String[] resource_array = new String[result.size()];
            return result.toArray(resource_array);
        }

        private BvPermissionRequest mBvPermissionRequest;
        private final String[] mResources;

        public PermissionRequestAdapter(BvPermissionRequest permissionRequest) {
            assert permissionRequest != null;
            mBvPermissionRequest = permissionRequest;
            mResources = toPermissionResources(mBvPermissionRequest.getResources());
        }

        @Override
        public Uri getOrigin() {
            return mBvPermissionRequest.getOrigin();
        }

        @Override
        public String[] getResources() {
            return mResources.clone();
        }

        @Override
        public void grant(String[] resources) {
            long requestedResource = mBvPermissionRequest.getResources();
            if ((requestedResource & toBvPermissionResources(resources)) == requestedResource) {
                mBvPermissionRequest.grant();
            } else {
                mBvPermissionRequest.deny();
            }
        }

        @Override
        public void deny() {
            mBvPermissionRequest.deny();
        }
    }

    public static BisonWebChromeClient.FileChooserParams fromBvFileChooserParams(
            final BvContentsClient.FileChooserParamsImpl value) {
        if (value == null) {
            return null;
        }
        return new BisonWebChromeClient.FileChooserParams() {
            @Override
            public int getMode() {
                return value.getMode();
            }

            @Override
            public String[] getAcceptTypes() {
                return value.getAcceptTypes();
            }

            @Override
            public boolean isCaptureEnabled() {
                return value.isCaptureEnabled();
            }

            @Override
            public CharSequence getTitle() {
                return value.getTitle();
            }

            @Override
            public String getFilenameHint() {
                return value.getFilenameHint();
            }

            @Override
            public Intent createIntent() {
                return value.createIntent();
            }
        };
    }

    private static ConsoleMessage fromBvConsoleMessage(BvConsoleMessage value) {
        if (value == null) {
            return null;
        }
        return new ConsoleMessage(value.message(), value.sourceId(), value.lineNumber(),
                fromBisonMessageLevel(value.messageLevel()));
    }

    private static ConsoleMessage.MessageLevel fromBisonMessageLevel(
        @BvConsoleMessage.MessageLevel int value) {
        switch (value) {
            case BvConsoleMessage.MESSAGE_LEVEL_TIP:
                return ConsoleMessage.MessageLevel.TIP;
            case BvConsoleMessage.MESSAGE_LEVEL_LOG:
                return ConsoleMessage.MessageLevel.LOG;
            case BvConsoleMessage.MESSAGE_LEVEL_WARNING:
                return ConsoleMessage.MessageLevel.WARNING;
            case BvConsoleMessage.MESSAGE_LEVEL_ERROR:
                return ConsoleMessage.MessageLevel.ERROR;
            case BvConsoleMessage.MESSAGE_LEVEL_DEBUG:
                return ConsoleMessage.MessageLevel.DEBUG;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }

    @Override
    public final void overrideRequest(BvWebResourceRequest request) {
        mBisonViewClient.overrideRequest(mBisonView, new WebResourceRequestAdapter(request));
    }
}
