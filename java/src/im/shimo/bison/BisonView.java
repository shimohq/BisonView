package im.shimo.bison;

import android.content.Context;
import android.util.AttributeSet;
import android.webkit.ValueCallback;
import android.widget.FrameLayout;

import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.components.embedder_support.view.ContentViewRenderView;
import org.chromium.content_public.browser.BrowserStartupController;

public class BisonView extends FrameLayout implements BisonChromeEventListener, BisonContentsClientListener {

    static final BisonChromeClient sNullChromeClient = new BisonChromeClient();

    private ContentViewRenderView mContentViewRenderView;

    private BisonContents mBisonContents;
    private BisonChromeClient mBisonChromeClient = sNullChromeClient;
    private BisonViewClient mBisonViewClient;
    private BisonContentsClientBridge mBisonContentsClientBridge;

    /**
     * Constructor for inflating via XML.
     */
    public BisonView(Context context, AttributeSet attrs) {
        super(context, attrs);
        LibraryLoader.getInstance().ensureInitialized(LibraryProcessType.PROCESS_BROWSER);
        BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                .startBrowserProcessesSync(false);
        mBisonContentsClientBridge = new BisonContentsClientBridge(this);
        mBisonContents = new BisonContents(context, this, mBisonContentsClientBridge);
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

    public void evaluateJavascript(String script, ValueCallback<String> resultCallback) {
        mBisonContents.evaluateJavaScript(script, CallbackConverter.fromValueCallback(resultCallback));
    }

    public void setBisonViewClient(BisonViewClient client) {
        mBisonContents.setBisonViewClient(client);
    }

    public void setBisonChromeClient(BisonChromeClient client) {
        mBisonChromeClient = client;
    }

    public void destroy() {
        removeAllViews();
        if (mContentViewRenderView != null) {
            mContentViewRenderView.destroy();
            mContentViewRenderView = null;
        }
    }


    @Override
    public void onTitleUpdate(String title) {
        mBisonChromeClient.onReceivedTitle(this, title);
    }

    @Override
    public void onProgressChanged(int newProgress) {
        mBisonChromeClient.onProgressChanged(this, newProgress);
    }


    @Override
    public void onJsAlert(String url, String message, JsResult result) {
        mBisonChromeClient.onJsAlert(this, url, message, result);
    }

    @Override
    public void onJsConfirm(String url, String message, JsResult result) {
        mBisonChromeClient.onJsConfirm(this, url, message, result);
    }


}
