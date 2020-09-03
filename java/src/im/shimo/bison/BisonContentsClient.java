package im.shimo.bison;

import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.provider.Browser;
import android.view.WindowManager;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.content_public.common.ContentUrlConstants;

public class BisonContentsClient {

    private static final String TAG = "BisonContentsClient";

    static final BisonViewClient sNullBisonViewClient = new BisonViewClient();

    private BisonWebChromeClient mBisonWebChromeClient;
    private BisonViewClient mBisonViewClient = sNullBisonViewClient;
    private BisonView mBisonView;
    private Context mContext;

    public BisonContentsClient(BisonView bisonView, Context context) {
        mBisonView = bisonView;
        mContext = context;
    }

    public boolean hasBisonViewClient() {
        return sNullBisonViewClient != mBisonViewClient;
    }

    public void setBisonWebChromeClient(BisonWebChromeClient bisonWebChromeClient) {
        mBisonWebChromeClient = bisonWebChromeClient;
    }

    public void setBisonViewClient(BisonViewClient bisonViewClient) {
        mBisonViewClient = bisonViewClient;
    }

    public BisonViewClient getBisonViewClient() {
        return mBisonViewClient;
    }

    public void onProgressChanged(int progress) {
        if (mBisonWebChromeClient != null) {
            mBisonWebChromeClient.onProgressChanged(mBisonView, progress);
        }
    }


    public void handleJsAlert(String url, String message, JsResultReceiver receiver) {
        if (mBisonWebChromeClient != null) {
            final JsPromptResult res =
                    new JsPromptResultReceiverAdapter(receiver).getPromptResult();
            if (!mBisonWebChromeClient.onJsAlert(mBisonView, url, message, res)) {
                // showDefaultJsDialog
            }
        }
    }

    protected void handleJsBeforeUnload(String url, String message,
                                        JsResultReceiver receiver) {

    }

    protected void handleJsConfirm(String url, String message, JsResultReceiver receiver) {
        final JsPromptResult res =
                new JsPromptResultReceiverAdapter(receiver).getPromptResult();
        mBisonWebChromeClient.onJsConfirm(mBisonView, url, message, res);
    }

    protected void handleJsPrompt(String url, String message, String defaultValue,
                                  JsPromptResultReceiver receiver) {

    }

    private boolean showDefaultJsDialog(JsPromptResult res, int jsDialogType, String defaultValue,
                                        String message, String url) {
        // Note we must unwrap the Context here due to JsDialogHelper only using instanceof to
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
                    "Unable to create JsDialog. Has this WebView outlived the Activity it was created with?");
            return false;
        }
        return true;
    }

    public void onReceivedTitle(String title) {
        if (mBisonWebChromeClient != null) {
            mBisonWebChromeClient.onReceivedTitle(mBisonView, title);
        }
    }

    public void onPageStarted(String url) {
        // jiang 未完成
        mBisonViewClient.onPageStarted(mBisonView, url, null);
    }

    public void onPageFinished(String url) {
        // jiang 未完成
        mBisonViewClient.onPageFinished(mBisonView, url);
    }


    public final boolean shouldIgnoreNavigation(Context context, String url, boolean isMainFrame,
                                                boolean hasUserGesture, boolean isRedirect) {
//        AwContentsClientCallbackHelper.CancelCallbackPoller poller =
//                mCallbackHelper.getCancelCallbackPoller();
//        if (poller != null && poller.shouldCancelAllCallbacks()) return false;

        if (hasBisonViewClient()) {
            // Note: only GET requests can be overridden, so we hardcode the method.
            WebResourceRequest request =
                    new WebResourceRequest(url, isMainFrame, hasUserGesture, "GET", null);
            request.isRedirect = isRedirect;
            return shouldOverrideUrlLoading(request);
        } else {
            return sendBrowsingIntent(context, url, hasUserGesture, isRedirect);
        }
    }

    public final boolean shouldOverrideUrlLoading(WebResourceRequest request) {
        return mBisonViewClient.shouldOverrideUrlLoading(mBisonView, request);
    }



    private static boolean sendBrowsingIntent(Context context, String url, boolean hasUserGesture,
                                              boolean isRedirect) {
        if (!hasUserGesture && !isRedirect) {
            Log.w(TAG, "Denied starting an intent without a user gesture, URI %s", url);
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
            Log.w(TAG, "Bad URI %s", url, ex);
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
            Log.w(TAG, "Cannot call startActivity on non-activity context.");
            return false;
        }
        try {
            context.startActivity(intent);
            return true;
        } catch (ActivityNotFoundException ex) {
            Log.w(TAG, "No application can handle %s", url);
        } catch (SecurityException ex) {
            Log.w(TAG, "SecurityException when starting intent for %s", url);
        }

        return false;
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

}
