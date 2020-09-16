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
