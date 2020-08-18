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

import org.chromium.base.Callback;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.components.embedder_support.view.ContentView;
import org.chromium.components.embedder_support.view.ContentViewRenderView;
import org.chromium.content_public.browser.ActionModeCallbackHelper;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.NavigationController;
import org.chromium.content_public.browser.SelectionPopupController;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.ViewAndroidDelegate;
import org.chromium.ui.base.WindowAndroid;

@JNINamespace("bison")
class BisonContents {

    private long mNativeBisonContents;

    private WebContents mWebContents;


    public BisonContents() {
        mNativeBisonContents = BisonContentsJni.get().init(this);
        mWebContents = BisonContentsJni.get().getWebContents(mNativeBisonContents);
    }

    
    public WebContents getWebContents() {
        return mWebContents;
    }
    

    @CalledByNative
    private void onNativeDestroyed() {
        // mWindow = null;
        // mNativeBisonView = 0;
        // mWebContents = null;
    }


    @SuppressWarnings("unused")
    @CalledByNative
    private void onUpdateUrl(String url) {
        //mUrlTextView.setText(url);
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void onLoadProgressChanged(double progress) {
        //removeCallbacks(mClearProgressRunnable);
        // mProgressDrawable.setLevel((int) (10000.0 * progress));
        // if (progress == 1.0) postDelayed(mClearProgressRunnable, COMPLETED_PROGRESS_TIMEOUT_MS);
    }

    @CalledByNative
    private void toggleFullscreenModeForTab(boolean enterFullscreen) {
        // mIsFullscreen = enterFullscreen;
        // LinearLayout toolBar = (LinearLayout) findViewById(R.id.toolbar);
        // toolBar.setVisibility(enterFullscreen ? GONE : VISIBLE);
    }

    @CalledByNative
    private boolean isFullscreenForTabOrPending() {
        return false;
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void setIsLoading(boolean loading) {
        // mLoading = loading;
        // if (mLoading) {
        //     mStopReloadButton
        //             .setImageResource(android.R.drawable.ic_menu_close_clear_cancel);
        // } else {
        //     //mStopReloadButton.setImageResource(R.drawable.ic_refresh);
        // }
    }


    /**
     * Initializes the ContentView based on the native tab contents pointer passed in.
     * @param webContents A {@link WebContents} object.
     */
    @SuppressWarnings("unused")
    @CalledByNative
    private void initFromNativeTabContents(WebContents webContents) {
        // Context context = getContext();
        // ContentView cv = ContentView.createContentView(context, webContents);
        // mViewAndroidDelegate = new BisonViewAndroidDelegate(cv);
        // assert (mWebContents != webContents);
        // if (mWebContents != null) mWebContents.clearNativeReference();
        // webContents.initialize(
        //         "", mViewAndroidDelegate, cv, mWindow, WebContents.createDefaultInternalsHolder());
        // mWebContents = webContents;
        // SelectionPopupController.fromWebContents(webContents)
        //         .setActionModeCallback(defaultActionCallback());
        // mNavigationController = mWebContents.getNavigationController();
        // if (getParent() != null) mWebContents.onShow();
        // if (mWebContents.getVisibleUrl() != null) {
        //     //mUrlTextView.setText(mWebContents.getVisibleUrl());
        // }
        // // ((FrameLayout) findViewById(R.id.contentview_holder)).addView(cv,
        // //         new FrameLayout.LayoutParams(
        // //                 FrameLayout.LayoutParams.MATCH_PARENT,
        // //                 FrameLayout.LayoutParams.MATCH_PARENT));
        // addView(cv);
        // cv.requestFocus();
        // mContentViewRenderView.setCurrentWebContents(mWebContents);
    }


    @CalledByNative
    public void setOverlayMode(boolean useOverlayMode) {
        // mContentViewRenderView.setOverlayVideoMode(useOverlayMode);
        // if (mOverlayModeChangedCallbackForTesting != null) {
        //     mOverlayModeChangedCallbackForTesting.onResult(useOverlayMode);
        // }
    }

    @CalledByNative
    public void sizeTo(int width, int height) {
        // mWebContents.setSize(width, height);
    }

    
    /**
     * Enable/Disable navigation(Prev/Next) button if navigation is allowed/disallowed
     * in respective direction.
     * @param controlId Id of button to update
     * @param enabled enable/disable value
     */
    @CalledByNative
    private void enableUiControl(int controlId, boolean enabled) {
        // if (controlId == 0) {
        //     mPrevButton.setEnabled(enabled);
        // } else if (controlId == 1) {
        //     mNextButton.setEnabled(enabled);
        // }
    }


    @NativeMethods

    interface Natives {
        long init(BisonContents caller);
        WebContents getWebContents(long nativeBisonContents);
        void closeShell(long BisonViewPtr);
    }

}