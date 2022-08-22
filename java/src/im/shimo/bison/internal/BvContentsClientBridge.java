package im.shimo.bison.internal;

import java.lang.ref.WeakReference;
import java.security.Principal;
import java.security.PrivateKey;
import java.security.cert.CertificateEncodingException;
import java.security.cert.X509Certificate;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.security.auth.x500.X500Principal;

import org.chromium.base.Callback;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.CalledByNativeUnchecked;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.task.PostTask;
import org.chromium.components.embedder_support.util.WebResourceResponseInfo;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.net.NetError;

import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Handler;
import android.net.Uri;
import android.net.http.SslCertificate;
import android.net.http.SslError;

import androidx.annotation.RestrictTo;
import androidx.annotation.NonNull;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
@JNINamespace("bison")
public class BvContentsClientBridge {
    private static final String TAG = "BisonContentsClientBrid";

    private BvContentsClient mClient;
    private WeakReference<Context> mContextRef;
    // The native peer of this object.
    private long mNativeContentsClientBridge;

    private final ClientCertLookupTable mLookupTable;

    // Used for mocking this class in tests.
    protected BvContentsClientBridge(ClientCertLookupTable table) {
        mLookupTable = table;
    }

    public BvContentsClientBridge(Context context, BvContentsClient client,
            ClientCertLookupTable table) {
        assert client != null;
        mContextRef = new WeakReference<>(context);
        mClient = client;
        mLookupTable = table;
    }

    /**
     * Callback to communicate clientcertificaterequest back to the
     * BisonContentsClientBridge.
     * The public methods should be called on UI thread.
     * A request can not be proceeded, ignored or canceled more than once. Doing
     * this
     * is a programming error and causes an exception.
     */
    public class ClientCertificateRequestCallback {

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
            if (mNativeContentsClientBridge == 0)
                return;
            BvContentsClientBridgeJni.get().provideClientCertificateResponse(
                    mNativeContentsClientBridge, BvContentsClientBridge.this, mId, certChain,
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
        final Callback<Boolean> callback = value -> PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT,
                () -> proceedSslError(value.booleanValue(), id));

        new Handler().post(() -> mClient.onReceivedSslError(callback, sslError));

        // Record UMA on ssl error
        // Use sparse histogram in case new values are added in future releases
        // RecordHistogram.recordSparseHistogram(
        // "Android.WebView.onReceivedSslError.ErrorCode", sslError.getPrimaryError());
        return true;
    }

    private void proceedSslError(boolean proceed, int id) {
        if (mNativeContentsClientBridge == 0)
            return;
        BvContentsClientBridgeJni.get().proceedSslError(
                mNativeContentsClientBridge, this, proceed, id);
    }

    // Intentionally not private for testing the native peer of this class.
    @CalledByNative
    protected void selectClientCertificate(final int id, final String[] keyTypes,
            byte[][] encodedPrincipals, final String host, final int port) {
        assert mNativeContentsClientBridge != 0;
        ClientCertLookupTable.Cert cert = mLookupTable.getCertData(host, port);
        if (mLookupTable.isDenied(host, port)) {
            BvContentsClientBridgeJni.get().provideClientCertificateResponse(
                    mNativeContentsClientBridge, this, id, null, null);
            return;
        }
        if (cert != null) {
            BvContentsClientBridgeJni.get().provideClientCertificateResponse(
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
                    BvContentsClientBridgeJni.get().provideClientCertificateResponse(
                            mNativeContentsClientBridge, this, id, null,
                            null);
                    return;
                }
            }

        }

        final ClientCertificateRequestCallback callback = new ClientCertificateRequestCallback(id, host, port);
        mClient.onReceivedClientCertRequest(callback, keyTypes, principals, host, port);

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

    // jiang handleJsBeforeUnload
    // @CalledByNative
    private void handleJsBeforeUnload(final String url, final String message, final int id) {
        // Post the application callback back to the current thread to ensure the
        // application
        // callback is executed without any native code on the stack. This so that any
        // exception
        // thrown by the application callback won't have to be propagated through a
        // native call
        // stack.
        new Handler().post(() -> {
            JsResultHandler handler = new JsResultHandler(this, id);
            mClient.handleJsBeforeUnload(url, message, handler);
        });
    }

    @CalledByNative
    private void newDownload(String url, String userAgent, String contentDisposition,
            String mimeType, long contentLength) {
        mClient.getCallbackHelper().postOnDownloadStart(
                url, userAgent, contentDisposition, mimeType, contentLength);
    }

    @CalledByNative
    private void newLoginRequest(String realm, String account, String args) {
        mClient.getCallbackHelper().postOnReceivedLoginRequest(realm, account, args);

        // Record UMA for onReceivedLoginRequest.
        // AwHistogramRecorder.recordCallbackInvocation(
        //         AwHistogramRecorder.WebViewCallbackType.ON_RECEIVED_LOGIN_REQUEST);
    }

    @CalledByNative
    private void onReceivedError(
            // WebResourceRequest
            String url, boolean isMainFrame, boolean hasUserGesture, boolean isRendererInitiated,
            String method, String[] requestHeaderNames, String[] requestHeaderValues,
            // WebResourceError
            @NetError int errorCode, String description) {
        BvWebResourceRequest request = new BvWebResourceRequest(
                url, isMainFrame, hasUserGesture, method, requestHeaderNames, requestHeaderValues);
        BvContentsClient.BvWebResourceError error = new BvContentsClient.BvWebResourceError();
        error.errorCode = errorCode;
        error.description = description;

        // String unreachableWebDataUrl = AwContentsStatics.getUnreachableWebDataUrl();
        // boolean isErrorUrl =
        // unreachableWebDataUrl != null && unreachableWebDataUrl.equals(request.url);

        if (error.errorCode != NetError.ERR_ABORTED) {
            // NetError.ERR_ABORTED error code is generated for the following reasons:
            // - WebView.stopLoading is called;
            // - the navigation is intercepted by the embedder via shouldOverrideUrlLoading;
            // - server returned 204 status (no content).
            //
            // BisonView does not notify the embedder of these situations using
            // this error code with the BisonViewClient.onReceivedError callback.
            error.errorCode = ErrorCodeConversionHelper.convertErrorCode(error.errorCode);
            if (request.isMainFrame) {
                mClient.getCallbackHelper().postOnPageStarted(request.url);
            }
            mClient.getCallbackHelper().postOnReceivedError(request, error);
            if (request.isMainFrame) {
                mClient.getCallbackHelper().postOnPageFinished(request.url);
            }
        }
    }

    @CalledByNative
    private void onReceivedHttpError(
            // WebResourceRequest
            String url, boolean isMainFrame, boolean hasUserGesture, String method,
            String[] requestHeaderNames, String[] requestHeaderValues,
            // WebResourceResponse
            String mimeType, String encoding, int statusCode, String reasonPhrase,
            String[] responseHeaderNames, String[] responseHeaderValues) {
        BvWebResourceRequest request = new BvWebResourceRequest(
                url, isMainFrame, hasUserGesture, method, requestHeaderNames, requestHeaderValues);
        Map<String, String> responseHeaders = new HashMap<String, String>(responseHeaderNames.length);
        // Note that we receive un-coalesced response header lines, thus we need to
        // combine
        // values for the same header.
        for (int i = 0; i < responseHeaderNames.length; ++i) {
            if (!responseHeaders.containsKey(responseHeaderNames[i])) {
                responseHeaders.put(responseHeaderNames[i], responseHeaderValues[i]);
            } else if (!responseHeaderValues[i].isEmpty()) {
                String currentValue = responseHeaders.get(responseHeaderNames[i]);
                if (!currentValue.isEmpty()) {
                    currentValue += ", ";
                }
                responseHeaders.put(responseHeaderNames[i], currentValue + responseHeaderValues[i]);
            }
        }
        BvWebResourceResponse response = new BvWebResourceResponse(
                mimeType, encoding, null, statusCode, reasonPhrase, responseHeaders);
        mClient.getCallbackHelper().postOnReceivedHttpError(request, response);

    }

    @CalledByNativeUnchecked
    private boolean shouldOverrideUrlLoading(
            String url, boolean hasUserGesture, boolean isRedirect, boolean isOutermostMainFrame) {
        if (mContextRef.get() == null) return false;
        return mClient.shouldIgnoreNavigation(
            mContextRef.get(), url, isOutermostMainFrame, hasUserGesture, isRedirect);
    }

    @CalledByNative
    private boolean sendBrowseIntent(String url) {
        if (mContextRef.get()==null){
            return false;
        }
        try {
            Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
            intent.addCategory(Intent.CATEGORY_BROWSABLE);
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R) {
                intent.setFlags(Intent.FLAG_ACTIVITY_REQUIRE_NON_BROWSER
                        | Intent.FLAG_ACTIVITY_REQUIRE_DEFAULT);
            } else {
                ResolveInfo bestActivity = getBestActivityForIntent(intent);
                if (bestActivity == null) {
                    return false;
                }
                intent.setComponent(new ComponentName(
                        bestActivity.activityInfo.packageName, bestActivity.activityInfo.name));
            }
            mContextRef.get().startActivity(intent);
            return true;
        } catch (ActivityNotFoundException e) {
            Log.w(TAG, "Could not find an application to handle : %s", url);
        } catch (Exception e) {
            Log.e(TAG, "Exception while sending browse Intent.", e);
        }
        return false;
    }

    private ResolveInfo getBestActivityForIntent(Intent intent) {
        assert mContextRef.get() != null ;
        List<ResolveInfo> resolveInfos = mContextRef.get().getPackageManager().queryIntentActivities(
                intent, PackageManager.GET_RESOLVED_FILTER);

        ResolveInfo bestActivity = null;
        final int n = resolveInfos.size();

        if (n == 1) {
            bestActivity = resolveInfos.get(0);
        } else if (n > 1) {
            ResolveInfo r0 = resolveInfos.get(0);
            ResolveInfo r1 = resolveInfos.get(1);

            // If the first activity has a higher priority, or a different
            // default, then it is always desirable to pick it, else there is a tie
            // between the first and second activity and we cant choose one(best one).
            if (r0.priority > r1.priority || r0.preferredOrder > r1.preferredOrder
                    || r0.isDefault != r1.isDefault) {
                bestActivity = resolveInfos.get(0);
            }
        }
        // Different cases due to which we return null from here
        // 1. There is no activity to handle this intent
        // 2. We can't come down to a single higher priority activity
        // 3. Best activity to handle is actually a browser.
        if (bestActivity == null) {
            return null;
        }
        return bestActivity;
    }

    void confirmJsResult(int id, String prompt) {
        if (mNativeContentsClientBridge == 0)
            return;
        BvContentsClientBridgeJni.get().confirmJsResult(
                mNativeContentsClientBridge, this, id, prompt);
    }

    void cancelJsResult(int id) {
        if (mNativeContentsClientBridge == 0)
            return;
        BvContentsClientBridgeJni.get().cancelJsResult(
                mNativeContentsClientBridge, this, id);
    }

    @NativeMethods
    interface Natives {

        void confirmJsResult(long nativeBvContentsClientBridge, BvContentsClientBridge caller, int id, String prompt);

        void cancelJsResult(long nativeBvContentsClientBridge, BvContentsClientBridge caller, int id);

        void proceedSslError(long nativeBvContentsClientBridge, BvContentsClientBridge caller, boolean proceed, int id);

        void provideClientCertificateResponse(long nativeBvContentsClientBridge,
                BvContentsClientBridge caller, int id, byte[][] certChain, PrivateKey androidKey);

    }

}
