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

public class BisonView extends FrameLayout {

    private WebContents mWebContents;
    private NavigationController mNavigationController;
    private EditText mUrlTextView;
    private ImageButton mPrevButton;
    private ImageButton mNextButton;
    private ImageButton mStopReloadButton;

    private ClipDrawable mProgressDrawable;

    
    private ContentViewRenderView mContentViewRenderView;
    private WindowAndroid mWindow;
    private BisonViewAndroidDelegate mViewAndroidDelegate;

    private boolean mLoading;
    private boolean mIsFullscreen;

    private Callback<Boolean> mOverlayModeChangedCallbackForTesting;

    private ViewGroup mContentViewHodler;

    private BisonContents mBisonContents;

    /**
     * Constructor for inflating via XML.
     */
    public BisonView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContentViewHodler = new FrameLayout(context);
        addView(mContentViewHodler,new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,FrameLayout.LayoutParams.MATCH_PARENT));
        
        mWindow = new ActivityWindowAndroid(getActivity(), true);
        mContentViewRenderView = new ContentViewRenderView(getContext());
        mContentViewRenderView.onNativeLibraryLoaded(mWindow);
        mWindow.setAnimationPlaceholderView(mContentViewRenderView.getSurfaceView());
        addView(mContentViewRenderView);
    }

    public void init() {
        mBisonContents = new BisonContents();
        initFromNativeTabContents(mBisonContents.getWebContents());
    }

    @Override
    public void addView(View child){
        FrameLayout.LayoutParams layoutParams = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT);
        if (mContentViewHodler != null){
            mContentViewHodler.addView(child,layoutParams);
        }else{
            super.addView(child,layoutParams);
        }
    }

    @Override
    public void removeView(View view) {
        if (mContentViewHodler !=null){
            mContentViewHodler.removeView(view);
        }else{
            super.removeView(view);
        }
        
    }

    @SuppressWarnings("unused")
    public Activity getActivity() {
        return (Activity)getContext();
    }

    
    public void close() {
        
    }

    public boolean isDestroyed() {
        return false;
    }


    public boolean isLoading() {
        return mLoading;
    }

    public void loadUrl(String url) {
        if (url == null) return;

        if (TextUtils.equals(url, mWebContents.getLastCommittedUrl())) {
            mNavigationController.reload(true);
        } else {
            mNavigationController.loadUrl(new LoadUrlParams(sanitizeUrl(url)));
        }
        //mUrlTextView.clearFocus();
        // TODO(aurimas): Remove this when crbug.com/174541 is fixed.
        getContentView().clearFocus();
        getContentView().requestFocus();
    }


    public static String sanitizeUrl(String url) {
        if (url == null) return null;
        if (url.startsWith("www.") || url.indexOf(":") == -1) url = "http://" + url;
        return url;
    }

    public void destroy(){
        removeAllViews();
        if (mContentViewRenderView != null ){
            mContentViewRenderView.destroy();
            mContentViewRenderView = null;
        }

    }

    public BisonViewAndroidDelegate getViewAndroidDelegate() {
        return mViewAndroidDelegate;
    }

    /**
     * Initializes the ContentView based on the native tab contents pointer passed in.
     * @param webContents A {@link WebContents} object.
     */
    @SuppressWarnings("unused")
    @CalledByNative
    private void initFromNativeTabContents(WebContents webContents) {
        Context context = getContext();
        ContentView cv = ContentView.createContentView(context, webContents);
        mViewAndroidDelegate = new BisonViewAndroidDelegate(cv);
        assert (mWebContents != webContents);
        if (mWebContents != null) mWebContents.clearNativeReference();
        webContents.initialize(
                "", mViewAndroidDelegate, cv, mWindow, WebContents.createDefaultInternalsHolder());
        mWebContents = webContents;
        SelectionPopupController.fromWebContents(webContents)
                .setActionModeCallback(defaultActionCallback());
        mNavigationController = mWebContents.getNavigationController();
        if (getParent() != null) mWebContents.onShow();
        if (mWebContents.getVisibleUrl() != null) {
            //mUrlTextView.setText(mWebContents.getVisibleUrl());
        }
        // ((FrameLayout) findViewById(R.id.contentview_holder)).addView(cv,
        //         new FrameLayout.LayoutParams(
        //                 FrameLayout.LayoutParams.MATCH_PARENT,
        //                 FrameLayout.LayoutParams.MATCH_PARENT));
        addView(cv);
        cv.requestFocus();
        mContentViewRenderView.setCurrentWebContents(mWebContents);
    }

    /**
     * {link @ActionMode.Callback} that uses the default implementation in
     * {@link SelectionPopupController}.
     */
    private ActionMode.Callback defaultActionCallback() {
        final ActionModeCallbackHelper helper =
                SelectionPopupController.fromWebContents(mWebContents)
                        .getActionModeCallbackHelper();

        return new ActionMode.Callback() {
            @Override
            public boolean onCreateActionMode(ActionMode mode, Menu menu) {
                helper.onCreateActionMode(mode, menu);
                return true;
            }

            @Override
            public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
                return helper.onPrepareActionMode(mode, menu);
            }

            @Override
            public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
                return helper.onActionItemClicked(mode, item);
            }

            @Override
            public void onDestroyActionMode(ActionMode mode) {
                helper.onDestroyActionMode();
            }
        };
    }


    public void setOverayModeChangedCallbackForTesting(Callback<Boolean> callback) {
        mOverlayModeChangedCallbackForTesting = callback;
    }


    public ViewGroup getContentView() {
        ViewAndroidDelegate viewDelegate = mWebContents.getViewAndroidDelegate();
        return viewDelegate != null ? viewDelegate.getContainerView() : null;
    }

    public WebContents getWebContents() {
        return mWebContents;
    }

    private void setKeyboardVisibilityForUrl(boolean visible) {
        InputMethodManager imm = (InputMethodManager) getContext().getSystemService(
                Context.INPUT_METHOD_SERVICE);
        if (visible) {
            imm.showSoftInput(mUrlTextView, InputMethodManager.SHOW_IMPLICIT);
        } else {
            imm.hideSoftInputFromWindow(mUrlTextView.getWindowToken(), 0);
        }
    }

    
}
