package im.shimo.bison;

import android.content.Context;
import android.util.Log;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.widget.FrameLayout;

import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.components.embedder_support.view.ContentViewRenderView;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

/**
 * Container and generator of ShellViews.
 */
@JNINamespace("bison")
public class BisonViewManager {
    private static final String TAG = "BisonViewManager";
    public static final String DEFAULT_URL = "http://www.baidu.com";
    private WindowAndroid mWindow;
    private BisonView mActiveShell;
    private Context mContext;


    // The target for all content rendering.
    private ContentViewRenderView mContentViewRenderView;

    /**
     * Constructor for inflating via XML.
     */
    public BisonViewManager(final Context context,BisonView bisonView) {
        // super(context, attrs);
        this.mContext = context;
        BisonViewManagerJni.get().init(this);
        // LayoutInflater inflater =
        //         (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        // mActiveShell = (BisonView) inflater.inflate(R.layout.shell_view, null);
        
        // addView(mActiveShell, new FrameLayout.LayoutParams(
        //         FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
        this.mActiveShell = bisonView;
    }

    public Context getContext(){
        return mContext;
    }


    public void setWindow(WindowAndroid window) {
        assert window != null;
        mWindow = window;
        mContentViewRenderView = new ContentViewRenderView(getContext());
        mContentViewRenderView.onNativeLibraryLoaded(window);
    }


    public void loadUrl(String url) {
        if (mActiveShell!=null){
            mActiveShell.loadUrl(url);
        }
    }


    public BisonView getActiveShell() {
        return mActiveShell;
    }


    public void launchShell() {
        ThreadUtils.assertOnUiThread();
        //BisonViewManagerJni.get().launchShell();
        mActiveShell.init();
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private Object createBisonView(long nativeBisonViewPtr) {
        Log.d(TAG,"createBisonView");
        
        showBisonView(mActiveShell);
        return mActiveShell;
    }

    private void showBisonView(BisonView bisonView) {
        bisonView.setContentViewRenderView(mContentViewRenderView);
        WebContents webContents = mActiveShell.getWebContents();
        // if (webContents != null) {
        //     mContentViewRenderView.setCurrentWebContents(webContents);
        //     webContents.onShow();
        // }
    }

    @CalledByNative
    private void removeBisonView(BisonView bisonView) {
        if (bisonView == mActiveShell) mActiveShell = null;
        if (bisonView.getParent() == null) return;
        bisonView.setContentViewRenderView(null);
        //removeView(bisonView);
    }

    @CalledByNative
    public void destroy() {
        if (mActiveShell != null) {
            removeBisonView(mActiveShell);
        }
        if (mContentViewRenderView != null) {
            mContentViewRenderView.destroy();
            mContentViewRenderView = null;
        }
    }

    @NativeMethods
    interface Natives {
        void init(Object bisonViewManagerInstance);
        void launchShell();
    }
}
