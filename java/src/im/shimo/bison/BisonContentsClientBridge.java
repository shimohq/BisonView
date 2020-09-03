package im.shimo.bison;

import android.content.Context;
import android.os.Handler;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.CalledByNativeUnchecked;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

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

        void confirmJsResult(long nativeBisonContentsClientBridge, BisonContentsClientBridge caller,
                             int id, String prompt);

        void cancelJsResult(
                long nativeBisonContentsClientBridge, BisonContentsClientBridge caller, int id);

    }


}
