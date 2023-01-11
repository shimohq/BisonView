package im.shimo.bison.internal;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Picture;
import android.net.Uri;
import android.net.http.SslError;
import android.os.Looper;
import android.os.Message;
import android.provider.Browser;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;

import im.shimo.bison.BisonWebChromeClient;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.metrics.ScopedSysTraceEvent;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.components.embedder_support.util.WebResourceResponseInfo;
import org.chromium.content_public.common.ContentUrlConstants;

import java.security.Principal;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Pattern;



@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public abstract class BvContentsClient {
    private static final String TAG = "BvContentsClient";
    private final BvContentsClientCallbackHelper mCallbackHelper;

    // Last background color reported from the renderer. Holds the sentinal value INVALID_COLOR
    // if not valid.
    private int mCachedRendererBackgroundColor = INVALID_COLOR;
    // Holds the last known page title. {@link ContentViewClient#onUpdateTitle} is unreliable,
    // particularly for navigating backwards and forwards in the history stack. Instead, the last
    // known document title is kept here, and the clients gets updated whenever the value has
    // actually changed. Blink also only sends updates when the document title have changed,
    // so behaviours are consistent.
    private String mTitle = "";

    private static final int INVALID_COLOR = 0;

    private static final Pattern FILE_ANDROID_ASSET_PATTERN =
            Pattern.compile("^file:///android_(asset|res)/.*");

    public BvContentsClient() {
        this(Looper.myLooper());
    }

    /**
     *
     * See {@link android.webkit.WebChromeClient}. */
    public interface CustomViewCallback {
        /* See {@link android.webkit.WebChromeClient}. */
        public void onCustomViewHidden();
    }

    // Alllow injection of the callback thread, for testing.
    public BvContentsClient(Looper looper) {
        try (ScopedSysTraceEvent e =
                        ScopedSysTraceEvent.scoped("BvContentsClient.constructorOneArg")) {
            mCallbackHelper = new BvContentsClientCallbackHelper(looper, this);
        }
    }

    final BvContentsClientCallbackHelper getCallbackHelper() {
        return mCallbackHelper;
    }

    final int getCachedRendererBackgroundColor() {
        assert isCachedRendererBackgroundColorValid();
        return mCachedRendererBackgroundColor;
    }

    final boolean isCachedRendererBackgroundColorValid() {
        return mCachedRendererBackgroundColor != INVALID_COLOR;
    }

    final void onBackgroundColorChanged(int color) {
        // Avoid storing the sentinal INVALID_COLOR (note that both 0 and 1 are both
        // fully transparent so this transpose makes no visible difference).
        mCachedRendererBackgroundColor = color == INVALID_COLOR ? 1 : color;
    }

    //--------------------------------------------------------------------------------------------
    //             BisonView specific methods that map directly to BisonViewClient / BisonWebChromeClient
    //--------------------------------------------------------------------------------------------

    /**
     * Parameters for {@link BvContentsClient#onReceivedError} method.
     */
    public static class BvWebResourceError {
        public @WebviewErrorCode int errorCode = WebviewErrorCode.ERROR_UNKNOWN;
        public String description;
    }

    /**
     * Allow default implementations in chromium code.
     */
    public abstract boolean hasBisonViewClient();

    public abstract void getVisitedHistory(Callback<String[]> callback);

    public abstract void doUpdateVisitedHistory(String url, boolean isReload);

    public abstract void onProgressChanged(int progress);

    public abstract WebResourceResponseInfo shouldInterceptRequest(
            BvWebResourceRequest request);

    public abstract void overrideRequest(BvWebResourceRequest request);

    public abstract boolean shouldOverrideKeyEvent(KeyEvent event);

    public abstract boolean shouldOverrideUrlLoading(BvWebResourceRequest request);

    public abstract void onLoadResource(String url);

    public abstract void onUnhandledKeyEvent(KeyEvent event);

    public abstract boolean onConsoleMessage(BvConsoleMessage consoleMessage);

    public abstract void onReceivedHttpAuthRequest(BvHttpAuthHandler handler,
            String host, String realm);

    public abstract void onReceivedSslError(Callback<Boolean> callback, SslError error);

    public abstract void onReceivedClientCertRequest(
            final BvContentsClientBridge.ClientCertificateRequestCallback callback,
            final String[] keyTypes, final Principal[] principals, final String host,
            final int port);

    public abstract void onReceivedLoginRequest(String realm, String account, String args);

    public abstract void onFormResubmission(Message dontResend, Message resend);

    public abstract void onDownloadStart(String url, String userAgent, String contentDisposition,
            String mimeType, long contentLength);

    public final boolean shouldIgnoreNavigation(Context context, String url, boolean isMainFrame,
            boolean hasUserGesture, boolean isRedirect) {
        BvContentsClientCallbackHelper.CancelCallbackPoller poller =
                mCallbackHelper.getCancelCallbackPoller();
        if (poller != null && poller.shouldCancelAllCallbacks()) return false;

        if (hasBisonViewClient()) {
            // Note: only GET requests can be overridden, so we hardcode the method.
            BvWebResourceRequest request =
                    new BvWebResourceRequest(url, isMainFrame, hasUserGesture, "GET", null);
            request.isRedirect = isRedirect;
            return shouldOverrideUrlLoading(request);
        } else {
            return sendBrowsingIntent(context, url, hasUserGesture, isRedirect);
        }
    }

    private static boolean sendBrowsingIntent(Context context, String url, boolean hasUserGesture,
            boolean isRedirect) {
        if (!hasUserGesture && !isRedirect) {
            Log.w(TAG, "Denied starting an intent without a user gesture, URI %s", url);
            return true;
        }

        // Treat some URLs as internal, always open them in the WebView:
        // * about: scheme URIs
        // * chrome:// scheme URIs
        // * file:///android_asset/ or file:///android_res/ URIs
        if (url.startsWith(ContentUrlConstants.ABOUT_URL_SHORT_PREFIX)
                || url.startsWith(UrlConstants.CHROME_URL_PREFIX)
                || FILE_ANDROID_ASSET_PATTERN.matcher(url).matches()) {
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
            // This can happen if the Activity is exported="true", guarded by a permission, and sets
            // up an intent filter matching this intent. This is a valid configuration for an
            // Activity, so instead of crashing, we catch the exception and do nothing. See
            // https://crbug.com/808494 and https://crbug.com/889300.
            Log.w(TAG, "SecurityException when starting intent for %s", url);
        }

        return false;
    }

    public static Uri[] parseFileChooserResult(int resultCode, Intent intent) {
        if (resultCode == Activity.RESULT_CANCELED) {
            return null;
        }
        Uri result =
                intent == null || resultCode != Activity.RESULT_OK ? null : intent.getData();

        Uri[] uris = null;
        if (result != null) {
            uris = new Uri[1];
            uris[0] = result;
        }
        return uris;
    }

    /**
     * Type adaptation class for {@link android.webkit.FileChooserParams}.
     */
    public static class FileChooserParamsImpl {
        private int mMode;
        private String mAcceptTypes;
        private String mTitle;
        private String mDefaultFilename;
        private boolean mCapture;
        private static final Map<String, String> sAcceptTypesMapping = new HashMap<String,
                String>() {
            {
                put("application/*", "application/*");
                put("audio/*", "audio/*");
                put("font/*", "font/*");
                put("image/*", "image/*");
                put("text/*", "text/*");
                put("video/*", "video/*");
                put(".aac", "audio/aac");
                put(".abw", "application/x-abiword");
                put(".arc", "application/x-freearc");
                put(".avif", "image/avif");
                put(".avi", "video/x-msvideo");
                put(".azw", "application/vnd.amazon.ebook");
                put(".bin", "application/octet-stream");
                put(".bmp", "image/bmp");
                put(".bz", "application/x-bzip");
                put(".bz2", "application/x-bzip2");
                put(".cda", "application/x-cdf");
                put(".csh", "application/x-csh");
                put(".css", "text/css");
                put(".csv", "text/csv");
                put(".doc", "application/msword");
                put(".docx",
                        "application/vnd.openxmlformats-officedocument.wordprocessingml.document");
                put(".eot", "application/vnd.ms-fontobject");
                put(".epub", "application/epub+zip");
                put(".gz", "application/gzip");
                put(".gif", "image/gif");
                put(".htm", "text/html");
                put(".html", "text/html");
                put(".ico", "image/vnd.microsoft.icon");
                put(".ics", "text/calendar");
                put(".jar", "application/java-archive");
                put(".jpeg", "image/jpeg");
                put(".jpg", "image/jpeg");
                put(".js", "text/javascript");
                put(".json", "application/json");
                put(".jsonld", "application/ld+json");
                put(".mid", "audio/midi");
                put(".midi", "audio/midi");
                put(".mjs", "text/javascript");
                put(".mp3", "audio/mpeg");
                put(".mp4", "video/mp4");
                put(".mpeg", "video/mpeg");
                put(".mpkg", "application/vnd.apple.installer+xml");
                put(".odp", "application/vnd.oasis.opendocument.presentation");
                put(".ods", "application/vnd.oasis.opendocument.spreadsheet");
                put(".odt", "application/vnd.oasis.opendocument.text");
                put(".oga", "audio/ogg");
                put(".ogv", "video/ogg");
                put(".ogx", "application/ogg");
                put(".opus", "audio/opus");
                put(".otf", "font/otf");
                put(".png", "image/png");
                put(".pdf", "application/pdf");
                put(".php", "application/x-httpd-php");
                put(".ppt", "application/vnd.ms-powerpoint");
                put(".pptx",
                        "application/vnd.openxmlformats-officedocument"
                                + ".presentationml.presentation");
                put(".rar", "application/vnd.rar");
                put(".rtf", "application/rtf");
                put(".sh", "application/x-sh");
                put(".svg", "image/svg+xml");
                put(".swf", "application/x-shockwave-flash");
                put(".tar", "application/x-tar");
                put(".tif", "image/tiff");
                put(".tiff", "image/tiff");
                put(".ts", "video/mp2t");
                put(".ttf", "font/ttf");
                put(".txt", "text/plain");
                put(".vsd", "application/vnd.visio");
                put(".wav", "audio/wav");
                put(".weba", "audio/webm");
                put(".webm", "video/webm");
                put(".webp", "image/webp");
                put(".woff", "font/woff");
                put(".woff2", "font/woff2");
                put(".xhtml", "application/xhtml+xml");
                put(".xls", "application/vnd.ms-excel");
                put(".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
                put(".xml", "application/xml");
                put(".xul", "application/vnd.mozilla.xul+xml");
                put(".zip", "application/zip");
                put(".3gp", "video/3gpp");
                put(".3g2", "video/3gpp2");
                put(".7z", "application/x-7z-compressed");
            }
        };

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
            Intent i = new Intent(Intent.ACTION_GET_CONTENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);
            if (getMode() == BisonWebChromeClient.FileChooserParams.MODE_OPEN_MULTIPLE) {
                i.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true);
            }
            if (mAcceptTypes != null && !mAcceptTypes.trim().isEmpty()) {
                String[] acceptTypesArray = getAcceptTypes();
                if (acceptTypesArray.length > 0) {
                    String[] mimeTypesToAccept = getMimeTypesToAccept(getAcceptTypes());
                    if (mimeTypesToAccept.length > 0) {
                        if (!mimeTypesToAccept[0].trim().isEmpty()) {
                            mimeType = mimeTypesToAccept[0];
                        }
                        i.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypesToAccept);
                    }
                }
            }
            i.setType(mimeType);
            return i;
        }

        /**
         * This method takes a list of types to accept, which could be file extensions, MIME types,
         * or a sub-category of MIME types such as image/*, video/*, etc., and returns a list of
         * MIME types.
         * @param acceptTypesList
         * @return An array of MIME types to accept in the file selector
         */
        private String[] getMimeTypesToAccept(String[] acceptTypesList) {
            ArrayList<String> acceptTypesArray = new ArrayList<String>();
            for (int i = 0; i < acceptTypesList.length; i++) {
                if (sAcceptTypesMapping.containsKey(acceptTypesList[i])) {
                    acceptTypesArray.add(sAcceptTypesMapping.get(acceptTypesList[i]));
                } else if (sAcceptTypesMapping.containsValue(acceptTypesList[i])) {
                    // can also directly use the MIME type in the accept HTML field
                    acceptTypesArray.add(acceptTypesList[i]);
                }
            }
            return acceptTypesArray.toArray(new String[acceptTypesArray.size()]);
        }
    }

    public abstract void showFileChooser(
            Callback<String[]> uploadFilePathsCallback, FileChooserParamsImpl fileChooserParams);

    public abstract void onGeolocationPermissionsShowPrompt(
            String origin, BvGeolocationPermissions.Callback callback);

    public abstract void onGeolocationPermissionsHidePrompt();

    public abstract void onPermissionRequest(BvPermissionRequest bvPermissionRequest);

    public abstract void onPermissionRequestCanceled(BvPermissionRequest bvPermissionRequest);

    public abstract void onScaleChangedScaled(float oldScale, float newScale);

    protected abstract void handleJsAlert(String url, String message, JsResultReceiver receiver);

    protected abstract void handleJsBeforeUnload(String url, String message,
            JsResultReceiver receiver);

    protected abstract void handleJsConfirm(String url, String message, JsResultReceiver receiver);

    protected abstract void handleJsPrompt(String url, String message, String defaultValue,
            JsPromptResultReceiver receiver);

    protected abstract boolean onCreateWindow(String url ,boolean isDialog, boolean isUserGesture);

    protected abstract void onCloseWindow();

    public abstract void onReceivedTouchIconUrl(String url, boolean precomposed);

    public abstract void onReceivedIcon(Bitmap bitmap);

    public abstract void onReceivedTitle(String title);

    protected abstract void onRequestFocus();

    protected abstract View getVideoLoadingProgressView();

    public abstract void onPageStarted(String url);

    public abstract void onPageFinished(String url);

    public abstract void onPageCommitVisible(String url);

    public final void onReceivedError(BvWebResourceRequest request, BvWebResourceError error) {
        if (request.isMainFrame) {
            onReceivedError(error.errorCode, error.description, request.url);
        }
        onReceivedError2(request, error);

        // jiang 有空研究下 RecordHistogram
        // Record UMA on error code distribution here.
        // RecordHistogram.recordSparseHistogram(
        //         "Android.WebView.onReceivedError.ErrorCode", error.errorCode);
    }

    protected abstract void onReceivedError(int errorCode, String description, String failingUrl);

    protected abstract void onReceivedError2(
            BvWebResourceRequest request, BvWebResourceError error);


    public abstract void onReceivedHttpError(BvWebResourceRequest request,
            BvWebResourceResponse response);

    public abstract void onShowCustomView(View view, CustomViewCallback callback);

    public abstract void onHideCustomView();

    public abstract Bitmap getDefaultVideoPoster();

    //--------------------------------------------------------------------------------------------
    //                              Other
    //--------------------------------------------------------------------------------------------
    //
    public abstract void onFindResultReceived(int activeMatchOrdinal, int numberOfMatches,
            boolean isDoneCounting);

    /**
     * Called whenever there is a new content picture available.
     * @param picture New picture.
     */
    public abstract void onNewPicture(Picture picture);

    public final void updateTitle(String title, boolean forceNotification) {
        if (!forceNotification && TextUtils.equals(mTitle, title)) return;
        mTitle = title;
        mCallbackHelper.postOnReceivedTitle(mTitle);
    }

    public abstract void onRendererUnresponsive(BvRenderProcess renderProcess);
    public abstract void onRendererResponsive(BvRenderProcess renderProcess);

    public abstract boolean onRenderProcessGone(BvRenderProcessGoneDetail detail);
}
