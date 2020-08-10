package im.shimo.bison;

import android.content.Context;
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
public class BisonViewManager extends FrameLayout {

    public static final String DEFAULT_URL = "http://www.baidu.com";
    private WindowAndroid mWindow;
    private BisonView mActiveShell;

    private String mStartupUrl = DEFAULT_URL;

    // The target for all content rendering.
    private ContentViewRenderView mContentViewRenderView;

    /**
     * Constructor for inflating via XML.
     */
    public BisonViewManager(final Context context, AttributeSet attrs) {
        super(context, attrs);
        BisonViewManagerJni.get().init(this);
    }


    public void setWindow(WindowAndroid window) {
        assert window != null;
        mWindow = window;
        mContentViewRenderView = new ContentViewRenderView(getContext());
        mContentViewRenderView.onNativeLibraryLoaded(window);
    }


    public WindowAndroid getWindow() {
        return mWindow;
    }

    /**
     * Get the ContentViewRenderView.
     */
    public ContentViewRenderView getContentViewRenderView() {
        return mContentViewRenderView;
    }


    public void setStartupUrl(String url) {
        mStartupUrl = url;
    }


    public BisonView getActiveShell() {
        return mActiveShell;
    }


    public void launchShell(String url) {
        ThreadUtils.assertOnUiThread();
        BisonView previousShell = mActiveShell;
        BisonViewManagerJni.get().launchShell(url);
        if (previousShell != null) previousShell.close();
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private Object createBisonView(long nativeBisonViewPtr) {
        if (mContentViewRenderView == null) {
            mContentViewRenderView = new ContentViewRenderView(getContext());
            mContentViewRenderView.onNativeLibraryLoaded(mWindow);
        }
        LayoutInflater inflater =
                (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        BisonView bisonView = (BisonView) inflater.inflate(R.layout.shell_view, null);
        bisonView.initialize(nativeBisonViewPtr, mWindow);

        // TODO(tedchoc): Allow switching back to these inactive shells.
        if (mActiveShell != null) removeBisonView(mActiveShell);

        showBisonView(bisonView);
        return bisonView;
    }

    private void showBisonView(BisonView bisonView) {
        bisonView.setContentViewRenderView(mContentViewRenderView);
        addView(bisonView, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
        mActiveShell = bisonView;
        WebContents webContents = mActiveShell.getWebContents();
        if (webContents != null) {
            mContentViewRenderView.setCurrentWebContents(webContents);
            webContents.onShow();
        }
    }

    @CalledByNative
    private void removeBisonView(BisonView bisonView) {
        if (bisonView == mActiveShell) mActiveShell = null;
        if (bisonView.getParent() == null) return;
        bisonView.setContentViewRenderView(null);
        removeView(bisonView);
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
        void launchShell(String url);
    }
}
