package im.shimo.bison;

import android.os.Handler;
import android.util.Log;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

@JNINamespace("bison")
class BisonContentsClientBridge {
    private static final String TAG = "BisonContentsClientBrid";

    private long mNativeContentsClientBridge;

    private BisonContentsClientListener mBisonContentsClientListener;

    public BisonContentsClientBridge(BisonContentsClientListener listener) {
        mBisonContentsClientListener = listener;
    }

    @CalledByNative
    private void setNativeContentsClientBridge(long nativeContentsClientBridge) {
        this.mNativeContentsClientBridge = nativeContentsClientBridge;
    }

    @CalledByNative
    private void handleJsAlert(final String url, final String message, final int id) {
        if (mBisonContentsClientListener != null) {
            new Handler().post(() -> {
                JsResultHandler handler = new JsResultHandler(this, id);

                final JsPromptResult res =
                        new JsPromptResultReceiverAdapter((JsResultReceiver) handler).getPromptResult();
                mBisonContentsClientListener.onJsAlert(url, message, res);
                Log.d(TAG, "handleJsAlert: ");
            });
        }

    }

    @CalledByNative
    private void handleJsConfirm(final String url, final String message, final int id) {
        if (mBisonContentsClientListener != null) {
            new Handler().post(() -> {
                JsResultHandler handler = new JsResultHandler(this, id);
                final JsPromptResult res =
                        new JsPromptResultReceiverAdapter((JsResultReceiver) handler).getPromptResult();
                mBisonContentsClientListener.onJsConfirm(url, message, res);
                Log.d(TAG, "handleJsConfirm: ");
            });
        }

    }

    @CalledByNative
    private void handleJsPrompt(
            final String url, final String message, final String defaultValue, final int id) {
        new Handler().post(() -> {
            JsResultHandler handler = new JsResultHandler(this, id);
            //mClient.handleJsPrompt(url, message, defaultValue, handler);
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


    @NativeMethods
    interface Natives {

        void confirmJsResult(long nativeBisonContentsClientBridge, BisonContentsClientBridge caller,
                             int id, String prompt);

        void cancelJsResult(
                long nativeBisonContentsClientBridge, BisonContentsClientBridge caller, int id);

    }


}
