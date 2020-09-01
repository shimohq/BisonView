package im.shimo.bison;

import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.provider.Browser;
import android.util.Log;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.CalledByNativeUnchecked;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.content_public.common.ContentUrlConstants;

@JNINamespace("bison")
class BisonContentsClientBridge {
    private static final String TAG = "BisonContentsClientBrid";

    private long mNativeContentsClientBridge;

    private Context mContext;
    private BisonContentsClientListener mBisonContentsClientListener;

    public BisonContentsClientBridge(Context context, BisonContentsClientListener listener) {
        this.mContext = context;
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
                
            });
        }

    }

    @CalledByNative
    private void handleJsPrompt(
            final String url, final String message, final String defaultValue, final int id) {
        if (mBisonContentsClientListener != null) {
            new Handler().post(() -> {
                JsResultHandler handler = new JsResultHandler(this, id);
                final JsPromptResult res =
                        new JsPromptResultReceiverAdapter((JsPromptResultReceiver) handler).getPromptResult();
                mBisonContentsClientListener.onJsPrompt(url, message,defaultValue, res);
            });
        }
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

     @CalledByNativeUnchecked
    private boolean shouldOverrideUrlLoading(
            String url, boolean hasUserGesture, boolean isRedirect, boolean isMainFrame) {
        // return mClient.shouldIgnoreNavigation(
        //         mContext, url, isMainFrame, hasUserGesture, isRedirect);
        Log.d(TAG, "shouldOverrideUrlLoading: ");
        // jiang hasBisonViewClient
        if (mBisonContentsClientListener != null) {

            WebResourceRequest request = new WebResourceRequest(url,isMainFrame,hasUserGesture,"GET",null);
            request.isRedirect = isRedirect;
            return mBisonContentsClientListener.shouldOverrideUrlLoading(request);
        } else {
            return sendBrowsingIntent(mContext, url, hasUserGesture, isRedirect);
        }
    }

     private static boolean sendBrowsingIntent(Context context, String url, boolean hasUserGesture,
            boolean isRedirect) {
        if (!hasUserGesture && !isRedirect) {
            //Log.w(TAG, "Denied starting an intent without a user gesture, URI %s", url);
            return true;
        }

        // Treat 'about:' URLs as internal, always open them in the WebView
        if (url.startsWith(ContentUrlConstants.ABOUT_URL_SHORT_PREFIX)) {
            return false;
        }

        Intent intent;
        // Perform generic parsing of the URI to turn it into an Intent.
        try {
            intent = Intent.parseUri(url, Intent.URI_INTENT_SCHEME);
        } catch (Exception ex) {
            //Log.w(TAG, "Bad URI %s", url, ex);
            return false;
        }
        // Sanitize the Intent, ensuring web pages can not bypass browser
        // security (only access to BROWSABLE activities).
        intent.addCategory(Intent.CATEGORY_BROWSABLE);
        intent.setComponent(null);
        Intent selector = intent.getSelector();
        if (selector != null) {
            selector.addCategory(Intent.CATEGORY_BROWSABLE);
            selector.setComponent(null);
        }

        // Pass the package name as application ID so that the intent from the
        // same application can be opened in the same tab.
        intent.putExtra(Browser.EXTRA_APPLICATION_ID, context.getPackageName());

        // Check whether the context is activity context.
        if (ContextUtils.activityFromContext(context) == null) {
            //Log.w(TAG, "Cannot call startActivity on non-activity context.");
            return false;
        }

        try {
            context.startActivity(intent);
            return true;
        } catch (ActivityNotFoundException ex) {
            //Log.w(TAG, "No application can handle %s", url);
        } catch (SecurityException ex) {
            //Log.w(TAG, "SecurityException when starting intent for %s", url);
        }

        return false;
    }


    @NativeMethods
    interface Natives {

        void confirmJsResult(long nativeBisonContentsClientBridge, BisonContentsClientBridge caller,
                             int id, String prompt);

        void cancelJsResult(
                long nativeBisonContentsClientBridge, BisonContentsClientBridge caller, int id);

    }


}
