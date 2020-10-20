package im.shimo.bison;

import android.content.Context;
import android.os.Message;
import android.util.AttributeSet;
import android.widget.FrameLayout;

import androidx.annotation.Nullable;

import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.content_public.browser.BrowserStartupController;
import org.chromium.content_public.browser.ChildProcessCreationParams;

public class BisonView extends FrameLayout {

    private static ClientCertLookupTable sClientCertLookupTable;

    private BisonContentsClient mBisonContentsClient;
    private BisonContents mBisonContents;
    private BisonContentsClientBridge mBisonContentsClientBridge;

    private static BisonDevToolsServer gBisonDevToolsServer;

    public BisonView(Context context) {
        super(context);
        init(context);
    }

    public BisonView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    private void init(Context context) {
        ChildProcessCreationParams.set(context.getPackageName(), false,
                LibraryProcessType.PROCESS_WEBVIEW_CHILD, true,
                true, "im.shimo.bison.PrivilegedProcessService",
                "im.shimo.bison.SandboxedProcessService");
        BisonResources.resetIds(context);
        LibraryLoader.getInstance().ensureInitialized(LibraryProcessType.PROCESS_WEBVIEW);
        BrowserStartupController.get(LibraryProcessType.PROCESS_WEBVIEW)
                .startBrowserProcessesSync(false);
        mBisonContentsClient = new BisonContentsClient(this, context);
        mBisonContentsClientBridge = new BisonContentsClientBridge(context, mBisonContentsClient, getClientCertLookupTable());
        mBisonContents = new BisonContents(context, BisonBrowserContext.getDefault(), mBisonContentsClientBridge, 
                mBisonContentsClient);
        addView(mBisonContents);
    }

    public boolean isDestroyed() {
        return false;
    }

    public void loadUrl(String url) {
        mBisonContents.loadUrl(url);
    }

    public void postUrl(String url, byte[] postData) {
        mBisonContents.postUrl(url, postData);
    }

    public void loadData(String data, String mimeType, String encoding) {
        mBisonContents.loadData(data, mimeType, encoding);
    }

    public void loadDataWithBaseURL(String baseUrl, String data,
                                    String mimeType, String encoding, String failUrl) {
        mBisonContents.loadData(baseUrl, data, mimeType, encoding, failUrl);
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

    public String getTitle() {
        return mBisonContents.getTitle();
    }

    public String getUrl() {
        return mBisonContents.getUrl();
    }


    public void evaluateJavascript(String script, ValueCallback<String> resultCallback) {
        mBisonContents.evaluateJavaScript(script, CallbackConverter.fromValueCallback(resultCallback));
    }

    public void saveWebArchive(String filename) {
        saveWebArchive(filename, false, null);
    }

    public void saveWebArchive(String basename, boolean autoname, @Nullable ValueCallback<String> callback) {
        mBisonContents.saveWebArchive(basename, autoname, CallbackConverter.fromValueCallback(callback));
    }

    public void setFindListener(FindListener listener) {
        mBisonContentsClient.setFindListener(listener);
    }

    public void documentHasImages(Message response) {
        mBisonContents.documentHasImages(response);
    }

    public void findNext(boolean forward) {
        mBisonContents.findNext(forward);
    }

    public void findAllAsync(String find) {
        mBisonContents.findAllAsync(find);
    }

    public void setBisonViewClient(BisonViewClient client) {
        mBisonContentsClient.setBisonViewClient(client);
    }

    public void setBisonWebChromeClient(BisonWebChromeClient client) {
        mBisonContentsClient.setBisonWebChromeClient(client);
    }

    public void setDownloadListener(DownloadListener listener) {
        mBisonContentsClient.setDownloadListener(listener);
    }

    public void pauseTimers() {
        mBisonContents.pauseTimers();
    }

    public void resumeTimers() {
        mBisonContents.resumeTimers();
    }


    public void addJavascriptInterface(Object obj, String interfaceName) {
        mBisonContents.addJavascriptInterface(obj, interfaceName);
    }

    public BisonSettings getSettings() {
        return mBisonContents.getSettings();
    }

    public void destroy() {
        mBisonContents.destroy();
        removeAllViews();
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
