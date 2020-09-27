package im.shimo.bison;

import android.content.Context;
import android.net.http.SslCertificate;
import android.net.http.SslError;
import android.os.Handler;
import android.util.Log;

import org.chromium.base.Callback;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.CalledByNativeUnchecked;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.base.task.PostTask;
import org.chromium.net.NetError;


import java.security.PrivateKey;

@JNINamespace("bison")
class BisonContentsClientBridge {
    private static final String TAG = "BisonContentsClientBrid";

    private long mNativeContentsClientBridge;

    private Context mContext;
    private BisonContentsClient mClient;

    public BisonContentsClientBridge(Context context, BisonContentsClient client) {
        this.mContext = context;
        mClient = client;
    }

    

    @CalledByNative
    private void setNativeContentsClientBridge(long nativeContentsClientBridge) {
        this.mNativeContentsClientBridge = nativeContentsClientBridge;
    }

     @CalledByNative
    private boolean allowCertificateError(int certError, byte[] derBytes, final String url,
            final int id) {
        final SslCertificate cert = SslUtil.getCertificateFromDerBytes(derBytes);
        if (cert == null) {    
            return false;
        }
        final SslError sslError = SslUtil.sslErrorFromNetErrorCode(certError, cert, url);
        final Callback<Boolean> callback = value
                -> PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT,
                        () -> proceedSslError(value.booleanValue(), id));
        
        //new Handler().post(() -> mClient.onReceivedSslError(callback, sslError));

        // Record UMA on ssl error
        // Use sparse histogram in case new values are added in future releases
        // RecordHistogram.recordSparseHistogram(
        //         "Android.WebView.onReceivedSslError.ErrorCode", sslError.getPrimaryError());
        return true;
    }

    private void proceedSslError(boolean proceed, int id) {
        if (mNativeContentsClientBridge == 0) return;
        BisonContentsClientBridgeJni.get().proceedSslError(
                mNativeContentsClientBridge, this, proceed, id);
    }

    @CalledByNative
    private void handleJsAlert(final String url, final String message, final int id) {
        new Handler().post(() -> {
            JsResultHandler handler = new JsResultHandler(this, id);
            mClient.handleJsAlert(url, message, handler);
        });

    }

    @CalledByNative
    private void handleJsConfirm(final String url, final String message, final int id) {
        new Handler().post(() -> {
            JsResultHandler handler = new JsResultHandler(this, id);
            mClient.handleJsConfirm(url, message, handler);
        });
    }

    @CalledByNative
    private void handleJsPrompt(
            final String url, final String message, final String defaultValue, final int id) {
        new Handler().post(() -> {
            JsResultHandler handler = new JsResultHandler(this, id);
            mClient.handleJsPrompt(url, message, defaultValue, handler);
        });
    }


    @CalledByNative
    private void newDownload(String url, String userAgent, String contentDisposition,
            String mimeType, long contentLength) {
        mClient.getCallbackHelper().postOnDownloadStart(
                url, userAgent, contentDisposition, mimeType, contentLength);

        // Record UMA for onDownloadStart.
        // AwHistogramRecorder.recordCallbackInvocation(
        //         AwHistogramRecorder.WebViewCallbackType.ON_DOWNLOAD_START);
    }

    @CalledByNative
    private void onReceivedError(
            // WebResourceRequest
            String url, boolean isMainFrame, boolean hasUserGesture, boolean isRendererInitiated,
            String method, String[] requestHeaderNames, String[] requestHeaderValues,
            @NetError int errorCode, String description) {
        // jiang 
        // BisonContentsClient.BisonWebResourceRequest request = new BisonContentsClient.BisonWebResourceRequest(
        //         url, isMainFrame, hasUserGesture, method, requestHeaderNames, requestHeaderValues);
        // BisonContentsClient.BisonWebResourceError error = new BisonContentsClient.BisonWebResourceError();
        // error.errorCode = errorCode;
        // error.description = description;

        // String unreachableWebDataUrl = AwContentsStatics.getUnreachableWebDataUrl();
        // boolean isErrorUrl =
        //         unreachableWebDataUrl != null && unreachableWebDataUrl.equals(request.url);

        // if (!isErrorUrl && error.errorCode != NetError.ERR_ABORTED) {
        //     // NetError.ERR_ABORTED error code is generated for the following reasons:
        //     // - WebView.stopLoading is called;
        //     // - the navigation is intercepted by the embedder via shouldOverrideUrlLoading;
        //     // - server returned 204 status (no content).
        //     //
        //     // Android WebView does not notify the embedder of these situations using
        //     // this error code with the WebViewClient.onReceivedError callback.
        //     error.errorCode = ErrorCodeConversionHelper.convertErrorCode(error.errorCode);
        //     if (request.isMainFrame
        //             && AwFeatureList.pageStartedOnCommitEnabled(isRendererInitiated)) {
        //         mClient.getCallbackHelper().postOnPageStarted(request.url);
        //     }
        //     mClient.getCallbackHelper().postOnReceivedError(request, error);
        //     if (request.isMainFrame) {
        //         // Need to call onPageFinished after onReceivedError for backwards compatibility
        //         // with the classic webview. See also AwWebContentsObserver.didFailLoad which is
        //         // used when we want to send onPageFinished alone.
        //         mClient.getCallbackHelper().postOnPageFinished(request.url);
        //     }
        // }
    }



    void confirmJsResult(int id, String prompt) {
        if (mNativeContentsClientBridge == 0) return;
        BisonContentsClientBridgeJni.get().confirmJsResult(
                mNativeContentsClientBridge, this, id, prompt);
    }

    void cancelJsResult(int id) {
        if (mNativeContentsClientBridge == 0) return;
        BisonContentsClientBridgeJni.get().cancelJsResult(
                mNativeContentsClientBridge, this, id);
    }

    @CalledByNativeUnchecked
    private boolean shouldOverrideUrlLoading(String url, boolean hasUserGesture,
                                             boolean isRedirect, boolean isMainFrame) {
        return mClient.shouldIgnoreNavigation(
                mContext, url, isMainFrame, hasUserGesture, isRedirect);
    }


    @NativeMethods
    interface Natives {

        void confirmJsResult(long nativeBisonContentsClientBridge, BisonContentsClientBridge caller, int id, String prompt);
        void cancelJsResult(long nativeBisonContentsClientBridge, BisonContentsClientBridge caller, int id);

        // void takeSafeBrowsingAction(long nativeBisonContentsClientBridge,
        //        BisonContentsClientBridge caller, int action, boolean reporting, int requestId);
        void proceedSslError(long nativeBisonContentsClientBridge, BisonContentsClientBridge caller, boolean proceed, int id);        
        void provideClientCertificateResponse(long nativeBisonContentsClientBridge,
                BisonContentsClientBridge caller, int id, byte[][] certChain, PrivateKey androidKey);
        
        
    }


}
