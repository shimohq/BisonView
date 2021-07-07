package im.shimo.bison;

import android.content.Context;
import android.graphics.Picture;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Message;
import android.util.AttributeSet;
import android.widget.FrameLayout;
import android.view.autofill.AutofillValue;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.os.StrictMode;

import android.webkit.WebView.VisualStateCallback;
import android.webkit.WebBackForwardList;

import androidx.annotation.Nullable;

import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.content_public.browser.BrowserStartupController;
import java.io.BufferedWriter;
import java.io.File;
import java.util.Map;

public class BisonView extends FrameLayout {

    private static final String HTTP_AUTH_DATABASE_FILE = "http_auth.db";
    private static ClientCertLookupTable sClientCertLookupTable;

    private static BisonDevToolsServer gBisonDevToolsServer;
    private static boolean loaded ;

    private BisonContentsClient mBisonContentsClient;
    private BisonContents mBisonContents;
    private BisonContentsClientBridge mBisonContentsClientBridge;
    private BisonWebStorage mBisonWebStorage;
    private BisonViewDatabase mBisonViewDatabase;


    public BisonView(Context context) {
        super(context);
        init(context);
    }

    public BisonView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }





    private void init(Context context) {
        if (!loaded){
            StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
            try {
                LibraryLoader.getInstance().setLibraryProcessType(LibraryProcessType.PROCESS_WEBVIEW);
                LibraryLoader.getInstance().loadNow();
                BrowserStartupController.getInstance().startBrowserProcessesSync(LibraryProcessType.PROCESS_WEBVIEW,false);
                loaded = true;
            } finally {
                StrictMode.setThreadPolicy(oldPolicy);
            }
        }
        mBisonContentsClient = new BisonContentsClient(this, context);
        mBisonContentsClientBridge = new BisonContentsClientBridge(context, mBisonContentsClient, getClientCertLookupTable());
        BisonBrowserContext bisonBrowserContext = BisonBrowserContext.getDefault();
        mBisonWebStorage = new BisonWebStorage(bisonBrowserContext.getQuotaManagerBridge());
        mBisonContents = new BisonContents(context, this, bisonBrowserContext, mBisonContentsClientBridge,
                mBisonContentsClient);
        addView(mBisonContents);
    }

    public boolean isDestroyed() {
        return false;
    }

    public void setHttpAuthUsernamePassword(
        final String host, final String realm, final String username, final String password) {
            getBisonViewDatabase().setHttpAuthUsernamePassword(host, realm, username, password);
    }

    public String[] getHttpAuthUsernamePassword(final String host, final String realm) {
        return getBisonViewDatabase().getHttpAuthUsernamePassword(host, realm);
    }

    public void destroy() {
        setBisonWebChromeClient(null);
        setBisonViewClient(null);
        mBisonContents.destroy();
        removeAllViews();
    }

    public void setNetworkAvailable(final boolean networkUp) {
        //mBisonContents.setNetworkAvailable(networkUp);
    }

    public boolean savePicture(Bundle b, File dest) {
      // Intentional no-op: hidden method on WebView.
      return false;
    }


    public boolean restorePicture(Bundle b, File src) {
      // Intentional no-op: hidden method on WebView.
      return false;
    }

    public void loadUrl(final String url, final Map<String, String> additionalHttpHeaders) {
      mBisonContents.loadUrl(url, additionalHttpHeaders);
    }

    public void loadUrl(final String url) {
        mBisonContents.loadUrl(url);
    }

    public void postUrl(final String url, final byte[] postData) {
        mBisonContents.postUrl(url, postData);
    }

    public void loadData(final String data,final String mimeType,final  String encoding) {
        mBisonContents.loadData(data, mimeType, encoding);
    }

    public void loadDataWithBaseURL(String baseUrl, String data,
                                    String mimeType, String encoding, String failUrl) {
        mBisonContents.loadData(baseUrl, data, mimeType, encoding, failUrl);
    }

    public void evaluateJavascript(
            final String script, ValueCallback<String> resultCallback) {
      mBisonContents.evaluateJavaScript(script, CallbackConverter.fromValueCallback(resultCallback));
    }

    public void saveWebArchive(String filename) {
        saveWebArchive(filename, false, null);
    }

    public void saveWebArchive(final String basename, final boolean autoname,
            final ValueCallback<String> callback) {
        mBisonContents.saveWebArchive(
                    basename, autoname, CallbackConverter.fromValueCallback(callback));
    }
    public void stopLoading() {
        mBisonContents.stopLoading();
    }

    public void reload() {
        mBisonContents.reload();
    }

    public boolean canGoBack() {
        return mBisonContents.canGoBack();
    }

    public void goBack() {
        mBisonContents.goBack();
    }

    public boolean canGoForward() {
        return mBisonContents.canGoForward();
    }

    public void goForward() {
        mBisonContents.goForward();
    }

    public boolean canGoBackOrForward(int steps) {
        return mBisonContents.canGoBackOrForward(steps);
    }

    public void goBackOrForward(int steps) {
        mBisonContents.goBackOrForward(steps);
    }

    public boolean isPrivateBrowsingEnabled() {
      // Not supported in this WebView implementation.
      return false;
    }

    public boolean pageUp(final boolean top) {
        // jiang 不好实现
        return false;
    }

    public boolean pageDown(final boolean bottom) {
        // jiang 不好实现
        return false;
    }

    @TargetApi(Build.VERSION_CODES.M)
    public void insertVisualStateCallback(
            final long requestId, final VisualStateCallback callback) {
        // jiang
    }

    public void clearView() {
        // jiang
    }

    public Picture capturePicture() {
        // jiang
    }

    public float getScale() {
        // jiang
    }

    public void setInitialScale(final int scaleInPercent) {
        // jiang
    }

    public void invokeZoomPicker() {
        // jiang
    }


    public void requestFocusNodeHref(final Message hrefMsg) {
        // jiang
    }

    public void requestImageRef(final Message msg) {
        // jiang
    }

    public String getUrl() {
        return mBisonContents.getUrl();
    }

    public String getOriginalUrl() {
        return mBisonContents.getOriginalUrl();
    }

    public String getTitle() {
        return mBisonContents.getTitle();
    }

    public Bitmap getFavicon() {
        // jiang
        return null;
    }

    @Override
    public String getTouchIconUrl() {
        // Intentional no-op: hidden method on WebView.
        return null;
    }

    public int getProgress() {
        if (mBisonContents == null) return 100;
        // No checkThread() because the value is cached java side (workaround for b/10533304).
        //jiang
        //return mBisonContents.getMostRecentProgress();
        return 0;
    }

    public int getContentHeight() {
        //jiang
        return 0;
    }

    public int getContentWidth() {
        //jiang
        return 0;
    }

    public void pauseTimers() {
        mBisonContents.pauseTimers();
    }

    public void resumeTimers() {
        mBisonContents.resumeTimers();
    }

    public void onPause() {
        mBisonContents.onPause();
    }

    public void onResume() {
        mBisonContents.onResume();
    }

    public boolean isPaused() {
        // jiang
        return false;
    }

    public void freeMemory() {
        // Intentional no-op. Memory is managed automatically by Chromium.
    }

    public void clearCache(final boolean includeDiskFiles) {
        mBisonContents.clearCache(includeDiskFiles);
    }

    public void clearFormData() {
        // jiang
        //mBisonContents.hideAutofillPopup();
    }

    public void clearHistory() {
        mBisonContents.clearHistory();
    }

    public void clearSslPreferences() {
        mBisonContents.clearSslPreferences();
    }

    public WebBackForwardList copyBackForwardList() {
        //jiang
        return null;
    }

    public void setFindListener(FindListener listener) {
        mBisonContentsClient.setFindListener(listener);
    }

    public void findNext(final boolean forwards) {
        mBisonContents.findNext(forward);
    }

    public int findAll(final String searchString) {
        findAllAsync(searchString);
        return 0;
    }

    public void findAllAsync(String find) {
        mBisonContents.findAllAsync(find);
    }



    public void documentHasImages(final Message response) {
        mBisonContents.documentHasImages(response);
    }


    public void setBisonViewClient(BisonViewClient client) {
        mBisonContentsClient.setBisonViewClient(client);
    }

    public BisonRenderProcess getBisonRenderProcess() {
        // jiang
        return null;
    }

    public void setBisonRenderProcessClient(BisonRenderProcessClient client){
        mBisonContentsClient.setBisonRenderProcessClient(client);
    }

    public BisonRenderProcessClient getBisonRenderProcessClient(){
        //jiang
        return null;
    }

    public void setDownloadListener(DownloadListener listener) {
        mBisonContentsClient.setDownloadListener(listener);
    }

    public void setBisonWebChromeClient(BisonWebChromeClient client) {
        mBisonContentsClient.setBisonWebChromeClient(client);
    }

    public BisonWebChromeClient getBisonWebChromeClient() {
        //jiang
        return null;
    }

    private boolean doesSupportFullscreen(BisonWebChromeClient client) {
        //jiang
        return false;
    }

    // public void setPictureListener(final WebView.PictureListener listener) {
    //     //jiang
    // }

    public void addJavascriptInterface(Object obj, String interfaceName) {
        mBisonContents.addJavascriptInterface(obj, interfaceName);
    }

    public void removeJavascriptInterface(final String interfaceName) {
        mBisonContents.removeJavascriptInterface(interfaceName);
    }

    // public WebMessagePort[] createWebMessageChannel() {
    //     recordWebViewApiCall(ApiCall.CREATE_WEBMESSAGE_CHANNEL);
    //     return WebMessagePortAdapter.fromMessagePorts(
    //             mSharedWebViewChromium.createWebMessageChannel());
    // }

    // @TargetApi(Build.VERSION_CODES.M)
    // public void postMessageToMainFrame(final WebMessage message, final Uri targetOrigin) {
    //     recordWebViewApiCall(ApiCall.POST_MESSAGE_TO_MAIN_FRAME);
    //     mSharedWebViewChromium.postMessageToMainFrame(message.getData(), targetOrigin.toString(),
    //             WebMessagePortAdapter.toMessagePorts(message.getPorts()));
    // }

    public BisonSettings getSettings() {
        return mBisonContents.getSettings();
    }

    public void setMapTrackballToArrowKeys(boolean setMap) {
        // This is a deprecated API: intentional no-op.
    }

    public void dumpViewHierarchyWithProperties(BufferedWriter out, int level) {
        // Intentional no-op
    }

    public View findHierarchyView(String className, int hashCode) {
        // Intentional no-op
        return null;
    }

    public void autofill(final SparseArray<AutofillValue> values) {
        // jiang
        mBisonContents.autofill(values);
    }

    @Override
    public void setBackgroundColor(int color) {
        super.setBackgroundColor(color);
        mBisonContents.setBackgroundColor(color);
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        return mBisonContents.onCreateInputConnection(outAttrs);
    }

    public BisonWebStorage getBisonWebStorage() {
        return mBisonWebStorage;
    }

    public BisonViewDatabase getBisonViewDatabase() {
        if (mBisonViewDatabase ==null){
            mBisonViewDatabase = new BisonViewDatabase(
                HttpAuthDatabase.newInstance(getContext(), HTTP_AUTH_DATABASE_FILE));
        }
        return mBisonViewDatabase;
    }

    public static void setRemoteDebuggingEnabled(boolean enable) {
        if (gBisonDevToolsServer == null) {
            if (!enable) return;
            gBisonDevToolsServer = new BisonDevToolsServer();
        }
        gBisonDevToolsServer.setRemoteDebuggingEnabled(enable);
        if (!enable) {
            gBisonDevToolsServer.destroy();
            gBisonDevToolsServer = null;
        }
    }

    private static ClientCertLookupTable getClientCertLookupTable() {
        if (sClientCertLookupTable == null) {
            sClientCertLookupTable = new ClientCertLookupTable();
        }
        return sClientCertLookupTable;
    }

    public void logCommandLineForDebugging(){
        BisonContents.logCommandLineForDebugging();
    }

    public interface FindListener {
        void onFindResultReceived(int activeMatchOrdinal, int numberOfMatches,
            boolean isDoneCounting);
    }

    public interface DownloadListener {

    /**
     * Notify the host application that a file should be downloaded
     * @param url The full url to the content that should be downloaded
     * @param userAgent the user agent to be used for the download.
     * @param contentDisposition Content-disposition http header, if
     *                           present.
     * @param mimetype The mimetype of the content reported by the server
     * @param contentLength The file size reported by the server
     */
    public void onDownloadStart(String url, String userAgent,
            String contentDisposition, String mimetype, long contentLength);

    }

}
