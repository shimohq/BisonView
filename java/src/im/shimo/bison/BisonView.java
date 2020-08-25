package im.shimo.bison;

import android.app.Activity;
import android.content.Context;
import android.graphics.drawable.ClipDrawable;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.ActionMode;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;
import android.webkit.ValueCallback;

import org.chromium.base.Callback;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.components.embedder_support.view.ContentView;
import org.chromium.components.embedder_support.view.ContentViewRenderView;
import org.chromium.content_public.browser.ActionModeCallbackHelper;
import org.chromium.content_public.browser.BrowserStartupController;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.NavigationController;
import org.chromium.content_public.browser.SelectionPopupController;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.ViewAndroidDelegate;
import org.chromium.ui.base.WindowAndroid;

public class BisonView extends FrameLayout {

    private WebContents mWebContents;
    private EditText mUrlTextView;

    
    private ContentViewRenderView mContentViewRenderView;
    private BisonViewAndroidDelegate mViewAndroidDelegate;

    private boolean mLoading;
    private boolean mIsFullscreen;

    private Callback<Boolean> mOverlayModeChangedCallbackForTesting;

    private BisonContents mBisonContents;

    /**
     * Constructor for inflating via XML.
     */
    public BisonView(Context context, AttributeSet attrs) {
        super(context, attrs);
        LibraryLoader.getInstance().ensureInitialized(LibraryProcessType.PROCESS_BROWSER);
        BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                    .startBrowserProcessesSync(false);
        mBisonContents = new BisonContents(context);        
        addView(mBisonContents);
    }

    public boolean isDestroyed() {
        return false;
    }


    public boolean isLoading() {
        return mLoading;
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
        mBisonContents.loadData(baseUrl,data, mimeType, encoding,failUrl);
    }

    public void evaluateJavascript(String script, ValueCallback<String> resultCallback){
        mBisonContents.evaluateJavaScript(script, CallbackConverter.fromValueCallback(resultCallback));
    }

    public void destroy(){
        removeAllViews();
        if (mContentViewRenderView != null ){
            mContentViewRenderView.destroy();
            mContentViewRenderView = null;
        }

    }

    
}
