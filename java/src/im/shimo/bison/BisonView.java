package im.shimo.bison;

import android.content.Context;
import android.util.AttributeSet;
import android.webkit.ValueCallback;
import android.widget.FrameLayout;

import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.components.embedder_support.view.ContentViewRenderView;
import org.chromium.content_public.browser.BrowserStartupController;

public class BisonView extends FrameLayout implements BisonChromeEventListener {
    
    private ContentViewRenderView mContentViewRenderView;

    private BisonContents mBisonContents;
    private BisonChromeClient mBisonChromeClient;

    /**
     * Constructor for inflating via XML.
     */
    public BisonView(Context context, AttributeSet attrs) {
        super(context, attrs);
        LibraryLoader.getInstance().ensureInitialized(LibraryProcessType.PROCESS_BROWSER);
        BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                .startBrowserProcessesSync(false);
        mBisonContents = new BisonContents(context, this);
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
        if (mBisonChromeClient != null) {
            mBisonChromeClient.onReceivedTitle(this, title);
        }
    }
}
