package im.shimo.bison;

import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.graphics.Picture;
import android.os.Looper;
import android.os.Message;
import android.provider.Browser;
import android.view.WindowManager;
import android.text.TextUtils;
import android.net.Uri;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.base.TraceEvent;
import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.TraceEvent;
import org.chromium.content_public.common.ContentUrlConstants;

import java.lang.ref.WeakReference;
import java.security.Principal;
import java.security.PrivateKey;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.WeakHashMap;

public class BisonContentsClient {
    private static final boolean TRACE = false;

    private static final String TAG = "BisonContentsClient";

    static final BisonViewClient sNullBisonViewClient = new BisonViewClient();

    private BisonWebChromeClient mBisonWebChromeClient;

    // The listener receiving find-in-page API results.
    private BisonView.FindListener mFindListener;

    private BisonViewClient mBisonViewClient = sNullBisonViewClient;
    private BisonView mBisonView;
    private Context mContext;
    private final BisonContentsClientCallbackHelper mCallbackHelper;

    private WeakHashMap<BisonPermissionRequest, WeakReference<PermissionRequestAdapter>>
            mOngoingPermissionRequests;
    private BisonView.DownloadListener mDownloadListener;

    private String mTitle = "";

    public BisonContentsClient(BisonView bisonView, Context context) {
        mBisonView = bisonView;
        mContext = context;
        mCallbackHelper = new BisonContentsClientCallbackHelper(Looper.myLooper(),this);
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

    void setDownloadListener(BisonView.DownloadListener listener) {
        mDownloadListener = listener;
    }

    void setFindListener(BisonView.FindListener listener) {
        mFindListener = listener;
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

    public void onGeolocationPermissionsShowPrompt(
            String origin, BisonGeolocationPermissions.Callback callback) {
        
        TraceEvent.begin("WebViewContentsClientAdapter.onGeolocationPermissionsShowPrompt");
        if (mBisonWebChromeClient == null) {
            callback.invoke(origin, false, false);
            return;
        }
        
        mBisonWebChromeClient.onGeolocationPermissionsShowPrompt(origin,
                callback == null ? null : (callbackOrigin, allow, retain)
                        -> callback.invoke(callbackOrigin, allow, retain));
    }

    public void onGeolocationPermissionsHidePrompt() {
        if (mBisonWebChromeClient != null) {
                // if (TRACE) Log.i(TAG, "onGeolocationPermissionsHidePrompt");
                mBisonWebChromeClient.onGeolocationPermissionsHidePrompt();
        }
    }

    public void onPermissionRequest(BisonPermissionRequest permissionRequest) {
        try {
            TraceEvent.begin("BisonContentsClient.onPermissionRequest");
            if (mBisonWebChromeClient != null) {
                // if (TRACE) Log.i(TAG, "onPermissionRequest");
                if (mOngoingPermissionRequests == null) {
                    mOngoingPermissionRequests = new WeakHashMap<BisonPermissionRequest,
                            WeakReference<PermissionRequestAdapter>>();
                }
                PermissionRequestAdapter adapter = new PermissionRequestAdapter(permissionRequest);
                mOngoingPermissionRequests.put(
                        permissionRequest, new WeakReference<PermissionRequestAdapter>(adapter));
                mBisonWebChromeClient.onPermissionRequest(adapter);
            } else {
                // By default, we deny the permission.
                permissionRequest.deny();
            }
        } finally {
            TraceEvent.end("BisonContentsClient.onPermissionRequest");
        }
    }

    public void onPermissionRequestCanceled(BisonPermissionRequest permissionRequest) {
        try {
            TraceEvent.begin("BisonContentsClient.onPermissionRequestCanceled");
            if (mBisonWebChromeClient != null && mOngoingPermissionRequests != null) {
                // if (TRACE) Log.i(TAG, "onPermissionRequestCanceled");
                WeakReference<PermissionRequestAdapter> weakRef =
                        mOngoingPermissionRequests.get(permissionRequest);
                // We don't hold strong reference to PermissionRequestAdpater and don't expect the
                // user only holds weak reference to it either, if so, user has no way to call
                // grant()/deny(), and no need to be notified the cancellation of request.
                if (weakRef != null) {
                    PermissionRequestAdapter adapter = weakRef.get();
                    if (adapter != null) mBisonWebChromeClient.onPermissionRequestCanceled(adapter);
                }
            }
        } finally {
            TraceEvent.end("BisonContentsClient.onPermissionRequestCanceled");
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
//        BisonContentsClientCallbackHelper.CancelCallbackPoller poller =
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

    public BisonWebResourceResponse shouldInterceptRequest(BisonWebResourceRequest request) {
        //mBisonViewClient.shouldOverrideUrlLoading(mBisonView,request)
        return null;
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

    public void postOnLoadResource(String url) {

    }

    public void postOnReceivedError(BisonWebResourceRequest request, BisonWebResourceError bisonWebResourceError) {

    }

    public void onLoadResource(String url) {

    }

    public void onDownloadStart(String url, String userAgent, String contentDisposition, String mimeType, long contentLength) {
        if (mDownloadListener != null) {
            mDownloadListener.onDownloadStart(
                    url, userAgent, contentDisposition, mimeType, contentLength);
        }
    }

    private static class ClientCertRequestImpl extends ClientCertRequest {
        private final BisonContentsClientBridge.ClientCertificateRequestCallback mCallback;
        private final String[] mKeyTypes;
        private final Principal[] mPrincipals;
        private final String mHost;
        private final int mPort;

        public ClientCertRequestImpl(
                BisonContentsClientBridge.ClientCertificateRequestCallback callback, String[] keyTypes,
                Principal[] principals, String host, int port) {
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
    
    public void onReceivedClientCertRequest(
            BisonContentsClientBridge.ClientCertificateRequestCallback callback, String[] keyTypes,
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

    public void onReceivedLoginRequest(String realm, String account, String args) {
    }

    public void onReceivedError(BisonWebResourceRequest request, BisonWebResourceError error) {
    }

    public void onSafeBrowsingHit(BisonWebResourceRequest request, int threatType, Callback<BisonSafeBrowsingResponse> callback) {
    }

    public void onNewPicture(Picture picture) {
    }

    public void onScaleChangedScaled(float oldScale, float newScale) {
    }

    public void onReceivedHttpError(BisonWebResourceRequest request, BisonWebResourceResponse response) {
    }

    public void doUpdateVisitedHistory(String url, boolean isReload) {
    }

    public void onFormResubmission(Message dontResend, Message resend) {
    }

    public BisonContentsClientCallbackHelper getCallbackHelper() {
        return mCallbackHelper;
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


    static class BisonWebResourceRequest {
        // Prefer using other constructors over this one.
        public BisonWebResourceRequest() {
        }

        public BisonWebResourceRequest(String url, boolean isMainFrame, boolean hasUserGesture,
                                       String method, @Nullable HashMap<String, String> requestHeaders) {
            this.url = url;
            this.isMainFrame = isMainFrame;
            this.hasUserGesture = hasUserGesture;
            // Note: we intentionally let isRedirect default initialize to false. This is because we
            // don't always know if this request is associated with a redirect or not.
            this.method = method;
            this.requestHeaders = requestHeaders;
        }

        public BisonWebResourceRequest(String url, boolean isMainFrame, boolean hasUserGesture,
                                       String method, @NonNull String[] requestHeaderNames,
                                       @NonNull String[] requestHeaderValues) {
            this(url, isMainFrame, hasUserGesture, method,
                    new HashMap<String, String>(requestHeaderValues.length));
            for (int i = 0; i < requestHeaderNames.length; ++i) {
                this.requestHeaders.put(requestHeaderNames[i], requestHeaderValues[i]);
            }
        }

        // Url of the request.
        public String url;
        // Is this for the main frame or a child iframe?
        public boolean isMainFrame;
        // Was a gesture associated with the request? Don't trust can easily be spoofed.
        public boolean hasUserGesture;
        // Was it a result of a server-side redirect?
        public boolean isRedirect;
        // Method used (GET/POST/OPTIONS)
        public String method;
        // Headers that would have been sent to server.
        public HashMap<String, String> requestHeaders;
    }

    public static class PermissionRequestAdapter extends PermissionRequest {

        private static long toBisonPermissionResources(String[] resources) {
            long result = 0;
            for (String resource : resources) {
                if (resource.equals(PermissionRequest.RESOURCE_VIDEO_CAPTURE)) {
                    result |= PermissionResource.VIDEO_CAPTURE;
                } else if (resource.equals(PermissionRequest.RESOURCE_AUDIO_CAPTURE)) {
                    result |= PermissionResource.AUDIO_CAPTURE;
                } else if (resource.equals(PermissionRequest.RESOURCE_PROTECTED_MEDIA_ID)) {
                    result |= PermissionResource.PROTECTED_MEDIA_ID;
                } else if (resource.equals(BisonPermissionRequest.RESOURCE_MIDI_SYSEX)) {
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
                result.add(BisonPermissionRequest.RESOURCE_MIDI_SYSEX);
            }
            String[] resource_array = new String[result.size()];
            return result.toArray(resource_array);
        }

        private BisonPermissionRequest mBisonPermissionRequest;
        private final String[] mResources;

        public PermissionRequestAdapter(BisonPermissionRequest permissionRequest) {
            assert permissionRequest != null;
            mBisonPermissionRequest = permissionRequest;
            mResources = toPermissionResources(mBisonPermissionRequest.getResources());
        }

        @Override
        public Uri getOrigin() {
            return mBisonPermissionRequest.getOrigin();
        }

        @Override
        public String[] getResources() {
            return mResources.clone();
        }

        @Override
        public void grant(String[] resources) {
            long requestedResource = mBisonPermissionRequest.getResources();
            if ((requestedResource & toBisonPermissionResources(resources)) == requestedResource) {
                mBisonPermissionRequest.grant();
            } else {
                mBisonPermissionRequest.deny();
            }
        }

        @Override
        public void deny() {
            mBisonPermissionRequest.deny();
        }
    }

    public static class BisonWebResourceError {
    }




    //region FileChooser

    public void showFileChooser(Callback<String[]> uploadFilePathsCallback, FileChooserParamsImpl fileChooserParams) {
        try {
            TraceEvent.begin("BisonContentsClient.showFileChooser");

        } finally {
            TraceEvent.end("BisonContentsClient.showFileChooser");
        }
    }

    static class FileChooserParamsImpl {
        private int mMode;
        private String mAcceptTypes;
        private String mTitle;
        private String mDefaultFilename;
        private boolean mCapture;

        public FileChooserParamsImpl(int mode, String acceptTypes, String title,
                String defaultFilename, boolean capture) {
            mMode = mode;
            mAcceptTypes = acceptTypes;
            mTitle = title;
            mDefaultFilename = defaultFilename;
            mCapture = capture;
        }

        public String getAcceptTypesString() {
            return mAcceptTypes;
        }

        public int getMode() {
            return mMode;
        }

        public String[] getAcceptTypes() {
            if (mAcceptTypes == null) {
                return new String[0];
            }
            return mAcceptTypes.split(",");
        }

        public boolean isCaptureEnabled() {
            return mCapture;
        }

        public CharSequence getTitle() {
            return mTitle;
        }

        public String getFilenameHint() {
            return mDefaultFilename;
        }

        public Intent createIntent() {
            String mimeType = "*/*";
            if (mAcceptTypes != null && !mAcceptTypes.trim().isEmpty()) {
                mimeType = mAcceptTypes.split(",")[0];
            }

            Intent i = new Intent(Intent.ACTION_GET_CONTENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);
            i.setType(mimeType);
            return i;
        }
    }

    //endregion

    public void onFindResultReceived(int activeMatchOrdinal, int numberOfMatches,
            boolean isDoneCounting){
        try {
            TraceEvent.begin("BisonContentsClient.onFindResultReceived");
            if (mFindListener == null) return;
            if (TRACE) Log.i(TAG, "onFindResultReceived");
            mFindListener.onFindResultReceived(activeMatchOrdinal, numberOfMatches, isDoneCounting);
        } finally {
            TraceEvent.end("BisonContentsClient.onFindResultReceived");
        }
    }

    public void onRequestFocus() {
         try {
            TraceEvent.begin("BisonContentsClient.onRequestFocus");
            if (mBisonWebChromeClient != null) {
                if (TRACE) Log.i(TAG, "onRequestFocus");
                mBisonWebChromeClient.onRequestFocus(mBisonView);
            }
        } finally {
            TraceEvent.end("BisonContentsClient.onRequestFocus");
        }
    }


































    public final void updateTitle(String title, boolean forceNotification) {
        if (!forceNotification && TextUtils.equals(mTitle, title)) return;
        mTitle = title;
        mCallbackHelper.postOnReceivedTitle(mTitle);
    }
    
    private BisonRenderProcessClient mBisonRenderProcessClient;

    public void setBisonRenderProcessClient(BisonRenderProcessClient client){
        mBisonRenderProcessClient = client;
    }

    public void onRendererUnresponsive(BisonRenderProcess renderProcess){
        if (mBisonRenderProcessClient != null ) {
            mBisonRenderProcessClient.onRenderProcessUnresponsive(mBisonView,renderProcess);
        }
    }

    public void onRendererResponsive(BisonRenderProcess renderProcess) {
        if (mBisonRenderProcessClient != null){
            mBisonRenderProcessClient.onRenderProcessResponsive(mBisonView,renderProcess);
        }
    }

    public boolean onRenderProcessGone(BisonRenderProcessGoneDetail detail){
        return mBisonViewClient.onRenderProcessGone(mBisonView,detail);
    }
}
