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
import org.chromium.base.task.PostTask;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.net.NetError;

import java.security.Principal;
import java.security.PrivateKey;
import java.security.cert.CertificateEncodingException;
import java.security.cert.X509Certificate;
import java.util.HashMap;
import java.util.Map;

import javax.security.auth.x500.X500Principal;

@JNINamespace("bison")
class BisonContentsClientBridge {
    private static final String TAG = "BisonContentsClientBrid";

    private BisonContentsClient mClient;
    private Context mContext;
    private long mNativeContentsClientBridge;
    private final ClientCertLookupTable mLookupTable;
    
    

    public BisonContentsClientBridge(Context context, BisonContentsClient client, 
            ClientCertLookupTable table) {
        assert client != null;
        mContext = context;
        mClient = client;
        mLookupTable = table;
    }

    /**
     * Callback to communicate clientcertificaterequest back to the BisonContentsClientBridge.
     * The public methods should be called on UI thread.
     * A request can not be proceeded, ignored  or canceled more than once. Doing this
     * is a programming error and causes an exception.
     */
    class ClientCertificateRequestCallback {

        private final int mId;
        private final String mHost;
        private final int mPort;
        private boolean mIsCalled;

        public ClientCertificateRequestCallback(int id, String host, int port) {
            mId = id;
            mHost = host;
            mPort = port;
        }

        public void proceed(final PrivateKey privateKey, final X509Certificate[] chain) {
            PostTask.runOrPostTask(
                    UiThreadTaskTraits.DEFAULT, () -> proceedOnUiThread(privateKey, chain));
        }

        public void ignore() {
            PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> ignoreOnUiThread());
        }

        public void cancel() {
            PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> cancelOnUiThread());
        }

        private void proceedOnUiThread(PrivateKey privateKey, X509Certificate[] chain) {
            checkIfCalled();

            if (privateKey == null || chain == null || chain.length == 0) {
                Log.w(TAG, "Empty client certificate chain?");
                provideResponse(null, null);
                return;
            }
            // Encode the certificate chain.
            byte[][] encodedChain = new byte[chain.length][];
            try {
                for (int i = 0; i < chain.length; ++i) {
                    encodedChain[i] = chain[i].getEncoded();
                }
            } catch (CertificateEncodingException e) {
                Log.w(TAG, "Could not retrieve encoded certificate chain: " + e);
                provideResponse(null, null);
                return;
            }
            mLookupTable.allow(mHost, mPort, privateKey, encodedChain);
            provideResponse(privateKey, encodedChain);
        }

        private void ignoreOnUiThread() {
            checkIfCalled();
            provideResponse(null, null);
        }

        private void cancelOnUiThread() {
            checkIfCalled();
            mLookupTable.deny(mHost, mPort);
            provideResponse(null, null);
        }

        private void checkIfCalled() {
            if (mIsCalled) {
                throw new IllegalStateException("The callback was already called.");
            }
            mIsCalled = true;
        }

        private void provideResponse(PrivateKey privateKey, byte[][] certChain) {
            if (mNativeContentsClientBridge == 0) return;
            BisonContentsClientBridgeJni.get().provideClientCertificateResponse(
                    mNativeContentsClientBridge, BisonContentsClientBridge.this, mId, certChain,
                    privateKey);
        }
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
            // if the certificate or the client is null, cancel the request    
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

    // Intentionally not private for testing the native peer of this class.
    @CalledByNative
    protected void selectClientCertificate(final int id, final String[] keyTypes,
            byte[][] encodedPrincipals, final String host, final int port) {
        assert mNativeContentsClientBridge != 0;
        ClientCertLookupTable.Cert cert = mLookupTable.getCertData(host, port);
        if (mLookupTable.isDenied(host, port)) {
            BisonContentsClientBridgeJni.get().provideClientCertificateResponse(
                    mNativeContentsClientBridge, this, id, null, null);
            return;
        }
        if (cert != null) {
            BisonContentsClientBridgeJni.get().provideClientCertificateResponse(
                    mNativeContentsClientBridge, this, id, cert.mCertChain,
                    cert.mPrivateKey);
            return;
        }
        // Build the list of principals from encoded versions.
        Principal[] principals = null;
        if (encodedPrincipals.length > 0) {
            principals = new X500Principal[encodedPrincipals.length];
            for (int n = 0; n < encodedPrincipals.length; n++) {
                try {
                    principals[n] = new X500Principal(encodedPrincipals[n]);
                } catch (IllegalArgumentException e) {
                    Log.w(TAG, "Exception while decoding issuers list: " + e);
                    BisonContentsClientBridgeJni.get().provideClientCertificateResponse(
                            mNativeContentsClientBridge, this, id, null,
                            null);
                    return;
                }
            }

        }

        final ClientCertificateRequestCallback callback =
                new ClientCertificateRequestCallback(id, host, port);
        //mClient.onReceivedClientCertRequest(callback, keyTypes, principals, host, port);

        // Record UMA for onReceivedClientCertRequest.
        // AwHistogramRecorder.recordCallbackInvocation(
        //         AwHistogramRecorder.WebViewCallbackType.ON_RECEIVED_CLIENT_CERT_REQUEST);
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
