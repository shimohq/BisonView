package im.shimo.bison;

import static androidx.annotation.RestrictTo.Scope.LIBRARY_GROUP;

import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Picture;
import android.graphics.Rect;
import android.net.http.SslCertificate;
import android.os.Bundle;
import android.os.Message;
import android.print.PrintDocumentAdapter;
import android.util.AttributeSet;
import android.view.DragEvent;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityNodeProvider;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.FrameLayout;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Map;
import java.util.concurrent.Executor;

import im.shimo.bison.adapter.BisonViewProvider;
import im.shimo.bison.internal.BvDevToolsServer;
import im.shimo.bison.R;

public class BisonView extends FrameLayout {

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ MODE_SURFACE_VIEW, MODE_SURFACE_VIEW })
    public @interface WebContentsRenderView {
    }

    public static final int MODE_SURFACE_VIEW = 0;
    public static final int MODE_TEXTURE_VIEW = 1;

    private static BvDevToolsServer gBvDevToolsServer;

    private boolean mAttachedContents;

    // private GeolocationPermissions mGeolocationPermissions;

    protected BisonViewProvider mProvider;

    @WebContentsRenderView
    private int mWebContentsRenderView;

    public BisonView(Context context) {
        super(context);
        initialize(context, null);
    }

    public BisonView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        initialize(context, attrs);
    }

    public BisonView(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        initialize(context, attrs);
    }

    private void initialize(Context context, @Nullable AttributeSet attrs) {
        if (isInEditMode())
            return;
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.BisonView);
        mWebContentsRenderView = a.getInt(R.styleable.BisonView_webContentsRenderView, 0);
        a.recycle();
        mProvider = new BisonViewProvider(this, mWebContentsRenderView, new InternalAccess());
        setOverScrollMode(View.OVER_SCROLL_ALWAYS);
        setFocusable(true);
        setFocusableInTouchMode(true);
    }

    @Nullable
    public SslCertificate getCertificate() {
        return mProvider.getCertificate();
    }

    // public void setHttpAuthUsernamePassword(final String host, final String
    // realm, final String username,
    // final String password) {
    // getBisonViewDatabase().setHttpAuthUsernamePassword(host, realm, username,
    // password);
    // }

    // public String[] getHttpAuthUsernamePassword(final String host, final String
    // realm) {
    // return getBisonViewDatabase().getHttpAuthUsernamePassword(host, realm);
    // }

    public void destroy() {
        mProvider.destroy();
        removeAllViews();
    }

    public void setNetworkAvailable(boolean networkUp) {
        mProvider.setNetworkAvailable(networkUp);
    }

    @Nullable
    public WebBackForwardList saveState(Bundle outState) {
        return mProvider.saveState(outState);
    }

    @Nullable
    public WebBackForwardList restoreState(Bundle inState) {
        return mProvider.restoreState(inState);
    }

    public void loadUrl(String url, Map<String, String> additionalHttpHeaders) {
        mProvider.loadUrl(url, additionalHttpHeaders);
    }

    public void loadUrl(String url) {
        mProvider.loadUrl(url);
    }

    public void postUrl(String url, byte[] postData) {
        mProvider.postUrl(url, postData);
    }

    public void loadData(String data, String mimeType, String encoding) {
        mProvider.loadData(data, mimeType, encoding);
    }

    public void loadDataWithBaseURL(String baseUrl, String data, String mimeType, String encoding, String failUrl) {
        mProvider.loadDataWithBaseURL(baseUrl, data, mimeType, encoding, failUrl);
    }

    public void evaluateJavascript(String script, ValueCallback<String> resultCallback) {
        mProvider.evaluateJavaScript(script, resultCallback);
    }

    /**
     * Saves the current view as a web archive.
     *
     * @param filename the filename where the archive should be placed
     */
    public void saveWebArchive(String filename) {
        saveWebArchive(filename, false, null);
    }

    /**
     * Saves the current view as a web archive.
     *
     * @param basename the filename where the archive should be placed
     * @param autoname if {@code false}, takes basename to be a file. If
     *                 {@code true}, basename is assumed to be a directory in which
     *                 a filename will be chosen according to the URL of the current
     *                 page.
     * @param callback called after the web archive has been saved. The parameter
     *                 for onReceiveValue will either be the filename under which
     *                 the file was saved, or {@code null} if saving the file
     *                 failed.
     */
    public void saveWebArchive(String basename, boolean autoname, @Nullable ValueCallback<String> callback) {
        mProvider.saveWebArchive(basename, autoname, callback);
    }

    public void stopLoading() {
        mProvider.stopLoading();
    }

    public void reload() {
        mProvider.reload();
    }

    public boolean canGoBack() {
        return mProvider.canGoBack();
    }

    public void goBack() {
        mProvider.goBack();
    }

    public boolean canGoForward() {
        return mProvider.canGoForward();
    }

    public void goForward() {
        mProvider.goForward();
    }

    // jiang pageUp
    /**
     * Scrolls the contents of this BisonView up by half the view size.
     *
     * @param top {@code true} to jump to the top of the page
     * @return {@code true} if the page was scrolled
     */
    // public boolean pageUp(boolean top) {
    // return mBisonContents.pageUp(top);
    // }

    // jiang pageDown
    // /**
    // * Scrolls the contents of this BisonView down by half the page size.
    // *
    // * @param bottom {@code true} to jump to bottom of page
    // * @return {@code true} if the page was scrolled
    // */
    // public boolean pageDown(boolean bottom) {
    // return mBisonContents.pageDown(bottom);
    // }

    /**
     * Gets whether the page can go back or forward the given number of steps.
     *
     * @param steps the negative or positive number of steps to move the history
     */
    public boolean canGoBackOrForward(int steps) {
        return mProvider.canGoBackOrForward(steps);
    }

    public void postVisualStateCallback(long requestId, VisualStateCallback callback) {
        mProvider.insertVisualStateCallback(requestId, callback);
    }

    /**
     * Goes to the history item that is the number of steps away from the current
     * item. Steps is negative if backward and positive if forward.
     *
     * @param steps the number of steps to take back or forward in the back forward
     *              list
     */
    public void goBackOrForward(int steps) {
        mProvider.goBackOrForward(steps);
    }

    // jiang public Picture capturePicture()

    public PrintDocumentAdapter createPrintDocumentAdapter(String documentName) {
        return mProvider.createPrintDocumentAdapter(documentName);
    }

    // jiang public float getScale()

    public void setInitialScale(int scaleInPercent) {
        // jiang need
        // getSettings().getBvSettings().setInitialPageScale(scaleInPercent);
    }

    // jiang invokeZoomPicker()

    public HitTestResult getHitTestResult() {
        return mProvider.getHitTestResult();
    }

    /**
     * Requests the anchor or image element URL at the last tapped point. If hrefMsg
     * is {@code null}, this method returns immediately and does not dispatch
     * hrefMsg to its target. If the tapped point hits an image, an anchor, or an
     * image in an anchor, the message associates strings in named keys in its data.
     * The value paired with the key may be an empty string.
     *
     * @param hrefMsg the message to be dispatched with the result of the request.
     *                The message data contains three keys. "url" returns the
     *                anchor's href attribute. "title" returns the anchor's text.
     *                "src" returns the image's src attribute.
     */
    public void requestFocusNodeHref(@Nullable Message hrefMsg) {
        mProvider.requestFocusNodeHref(hrefMsg);
    }

    /**
     * Requests the URL of the image last touched by the user. msg will be sent to
     * its target with a String representing the URL as its object.
     *
     * @param msg the message to be dispatched with the result of the request as the
     *            data member with "url" as key. The result can be {@code null}.
     */
    public void requestImageRef(Message msg) {
        mProvider.requestImageRef(msg);
    }

    public String getUrl() {
        return mProvider.getUrl();
    }

    /**
     * Gets the original URL for the current page. This is not always the same as
     * the URL passed to BisonViewClient.onPageStarted because although the load for
     * that URL has begun, the current page may not have changed. Also, there may
     * have been redirects resulting in a different URL to that originally
     * requested.
     *
     * @return the URL that was originally requested for the current page
     */
    public String getOriginalUrl() {
        return mProvider.getOriginalUrl();
    }

    /**
     * Gets the title for the current page. This is the title of the current page
     * until BisonViewClient.onReceivedTitle is called.
     *
     * @return the title for the current page
     */
    public String getTitle() {
        return mProvider.getTitle();
    }

    public Bitmap getFavicon() {
        return mProvider.getFavicon();
    }

    /**
     * Gets the progress for the current page.
     *
     * @return the progress for the current page between 0 and 100
     */
    public int getProgress() {
        return mProvider.getProgress();
    }

    // jiang getContentHeight
    // jiang getContentWidth

    public void pauseTimers() {
        mProvider.pauseTimers();
    }

    public void resumeTimers() {
        mProvider.resumeTimers();
    }

    public void onPause() {
        mProvider.onPause();
    }

    public void onResume() {
        mProvider.onResume();
    }

    public boolean isPaused() {
        return mProvider.isPaused();
    }

    /**
     * Clears the resource cache. Note that the cache is per-application, so this
     * will clear the cache for all BisonViews used.
     *
     * @param includeDiskFiles if {@code false}, only the RAM cache is cleared
     */
    public void clearCache(boolean includeDiskFiles) {
        mProvider.clearCache(includeDiskFiles);
    }

    /**
     * Removes the autocomplete popup from the currently focused form field, if
     * present. Note this only affects the display of the autocomplete popup, it
     * does not remove any saved form data from this BisonView's store. To do that,
     * use {@link BisonViewDatabase#clearFormData}.
     */
    public void clearFormData() {
        mProvider.clearFormData();
    }

    /**
     * Tells this BisonView to clear its internal back/forward list.
     */
    public void clearHistory() {
        mProvider.clearHistory();
    }

    /**
     * Clears the SSL preferences table stored in response to proceeding with SSL
     * certificate errors.
     */
    public void clearSslPreferences() {
        mProvider.clearSslPreferences();
    }

    // jiang clearClientCertPreferences

    public void setFindListener(FindListener listener) {
        mProvider.setFindListener(listener);
    }

    public void findNext(boolean forward) {
        mProvider.findNext(forward);
    }

    /**
     * Finds all instances of find on the page and highlights them. Notifies any
     * registered {@link FindListener}.
     *
     * @param find the string to find
     * @return the number of occurrences of the String "find" that were found
     * @see #setFindListener
     * @deprecated {@link #findAllAsync} is preferred.
     */
    public int findAll(String searchString) {
        findAllAsync(searchString);
        return 0;
    }

    public void findAllAsync(String find) {
        mProvider.findAllAsync(find);
    }

    /**
     * Clears the highlighting surrounding text matches created by
     * {@link #findAllAsync}.
     */
    public void clearMatches() {
        mProvider.clearMatches();
    }

    public void documentHasImages(Message response) {
        mProvider.documentHasImages(response);
    }

    /**
     * Sets the BisonViewClient that will receive various notifications and
     * requests. This will replace the current handler.
     *
     * @param client an implementation of BisonViewClient
     * @see #getBisonViewClient
     */
    public void setBisonViewClient(BisonViewClient client) {
        mProvider.setBisonViewClient(client);
    }

    /**
     * Gets the BisonViewClient.
     *
     * @return the BisonViewClient, or a default client if not yet set
     * @see #setBisonViewClient
     */
    public BisonViewClient getBisonViewClient() {
        return mProvider.getBisonViewClient();
    }

    public BisonViewRenderProcess getBisonViewRenderProcess() {
        return mProvider.getBisonViewRenderProcess();
    }

    public void setBisonViewRenderProcessClient(BisonViewRenderProcessClient client) {
        setBisonViewRenderProcessClient(null, client);
    }

    public void setBisonViewRenderProcessClient(Executor executor, BisonViewRenderProcessClient client) {
        mProvider.setBisonViewRenderProcessClient(executor, client);
    }

    public void setBisonWebChromeClient(BisonWebChromeClient client) {
        mProvider.setBisonWebChromeClient(client);
    }

    public void setDownloadListener(DownloadListener listener) {
        mProvider.setDownloadListener(listener);
    }

    public void addJavascriptInterface(Object obj, String interfaceName) {
        mProvider.addJavascriptInterface(obj, interfaceName);
    }

    public void removeJavascriptInterface(@NonNull String name) {
        mProvider.removeJavascriptInterface(name);
    }

    // jiang WebMessagePortAdapter
    // public WebMessagePort[] createWebMessageChannel() {
    // return
    // WebMessagePortAdapter.fromMessagePorts(mProvider.createMessageChannel());
    // }

    public BisonViewSettings getSettings() {
        return mProvider.getSettings();
    }

    @Override
    public void setBackgroundColor(int color) {
        if (mProvider != null) {
            mProvider.setBackgroundColor(color);
        } else {
            super.setBackgroundColor(color);
        }

    }

    public void setWebContentsRenderView(@WebContentsRenderView int renderView) {
        if (mWebContentsRenderView == renderView)
            return;
        mWebContentsRenderView = renderView;
        if (mProvider != null) {
            mProvider.setWebContentsRenderView(renderView);
        }
    }

    // findFocus
    @Override
    public boolean onCheckIsTextEditor() {
        if (mProvider != null) {
            return mProvider.onCheckIsTextEditor();
        } else {
            return super.onCheckIsTextEditor();
        }
    }

    @Override
    protected void onConfigurationChanged(Configuration newConfig) {
        if (mProvider != null) {
            mProvider.onConfigurationChanged(newConfig);
        } else {
            super.onConfigurationChanged(newConfig);
        }

    }

    @Override
    protected void onAttachedToWindow() {
        assert !mAttachedContents;
        super.onAttachedToWindow();
        if (mProvider != null) {
            mProvider.onAttachedToWindow();
        }
        mAttachedContents = true;
    }

    @Override
    protected void onDetachedFromWindow() {
        assert mAttachedContents;
        if (!isInEditMode()) {
            mProvider.onDetachedFromWindow();
        }
        mAttachedContents = false;
        super.onDetachedFromWindow();
    }

    // @Override
    // public void setLayoutParams(ViewGroup.LayoutParams params) {
    // mProvider.setLayoutParams(params);
    // }

    @Override
    public void setOverScrollMode(int mode) {
        super.setOverScrollMode(mode);
        if (mProvider != null) {
            mProvider.setOverScrollMode(mode);
        }
    }

    @Override
    public void setScrollBarStyle(int style) {
        if (mProvider != null) {
            mProvider.setScrollBarStyle(style);
        }
        super.setScrollBarStyle(style);
    }

    // setScrollBarStyle

    @Override
    protected int computeHorizontalScrollRange() {
        if (mProvider != null) {
            return mProvider.computeHorizontalScrollRange();
        } else {
            return super.computeHorizontalScrollRange();
        }
    }

    @Override
    protected int computeHorizontalScrollOffset() {
        if (mProvider != null) {
            return mProvider.computeHorizontalScrollOffset();
        } else {
            return computeHorizontalScrollOffset();
        }

    }

    /*
     * @Override
     * protected int computeHorizontalScrollExtent() {
     * return mProvider.computeHorizontalScrollExtent();
     * }
     */

    @Override
    protected int computeVerticalScrollRange() {
        if (mProvider != null) {
            return mProvider.computeVerticalScrollRange();
        } else {
            return super.computeVerticalScrollRange();
        }
    }

    @Override
    protected int computeVerticalScrollOffset() {
        if (mProvider != null) {
            return mProvider.computeVerticalScrollOffset();
        } else {
            return super.computeVerticalScrollOffset();
        }
    }

    @Override
    protected int computeVerticalScrollExtent() {
        if (mProvider != null) {
            return mProvider.computeVerticalScrollExtent();
        } else {
            return super.computeVerticalScrollExtent();
        }

    }

    // @Override
    // public void computeScroll() {
    // mProvider.computeScroll();
    // }

    @Override
    public boolean onHoverEvent(MotionEvent ev) {
        if (mProvider != null) {
            return mProvider.onHoverEvent(ev);
        } else {
            return super.onHoverEvent(ev);
        }

    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        if (mProvider != null) {
            return mProvider.onTouchEvent(ev);
        } else {
            return super.onTouchEvent(ev);
        }
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent ev) {
        if (mProvider != null) {
            return mProvider.onGenericMotionEvent(ev);
        } else {
            return super.onGenericMotionEvent(ev);
        }
    }

    // onTrackballEvent

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (mProvider != null) {
            return mProvider.onKeyDown(keyCode, event);
        } else {
            return super.onKeyDown(keyCode, event);
        }

    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (mProvider != null) {
            return mProvider.onKeyUp(keyCode, event);
        } else {
            return super.onKeyUp(keyCode, event);
        }

    }

    @Override
    public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event) {
        if (mProvider != null) {
            return mProvider.onKeyMultiple(keyCode, repeatCount, event);
        } else {
            return super.onKeyMultiple(keyCode, repeatCount, event);
        }

    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        if (mProvider != null) {
            return mProvider.onCreateInputConnection(outAttrs);
        } else {
            return super.onCreateInputConnection(outAttrs);
        }

    }

    @Override
    public void onWindowFocusChanged(boolean hasWindowFocus) {
        if (mProvider != null) {
            mProvider.onWindowFocusChanged(hasWindowFocus);
        }

        super.onWindowFocusChanged(hasWindowFocus);
    }

    @Override
    protected void onFocusChanged(boolean focused, int direction, Rect previouslyFocusedRect) {
        super.onFocusChanged(focused, direction, previouslyFocusedRect);
        if (mProvider != null) {
            mProvider.onFocusChanged(focused, direction, previouslyFocusedRect);
        }

    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (mProvider != null) {
            return mProvider.dispatchKeyEvent(event);
        } else {
            return super.dispatchKeyEvent(event);
        }

    }

    @Override
    public boolean requestFocus(int direction, Rect previouslyFocusedRect) {
        if (mProvider != null) {
            return mProvider.requestFocus(direction, previouslyFocusedRect);
        } else {
            return super.requestFocus(direction, previouslyFocusedRect);
        }

    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        if (mProvider != null) {
            mProvider.onMeasure(widthMeasureSpec, heightMeasureSpec);
        }

    }

    // @Override
    // public void onSizeChanged(int w, int h, int ow, int oh) {
    // super.onSizeChanged(w, h, ow, oh);
    // mProvider.onSizeChanged(w, h, ow, oh);
    // }

    @Override
    protected void onOverScrolled(int scrollX, int scrollY, boolean clampedX, boolean clampedY) {
        if (mProvider != null) {
            mProvider.onOverScrolled(scrollX, scrollY, clampedX, clampedY);
        } else {
            super.onOverScrolled(scrollX, scrollY, clampedX, clampedY);
        }

    }

    @Override
    protected void onScrollChanged(int l, int t, int oldl, int oldt) {
        super.onScrollChanged(l, t, oldl, oldt);
        // if (mProvider != null) {
        // mProvider.onScrollChanged(l, t, oldl, oldt);
        // }
    }

    @Override
    protected void onVisibilityChanged(View changedView, int visibility) {
        super.onVisibilityChanged(changedView, visibility);
        if (mProvider != null) {
            mProvider.onVisibilityChanged(changedView, visibility);
        }

    }

    @Override
    protected void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);
        if (mProvider != null) {
            mProvider.onWindowVisibilityChanged(visibility);
        }
    }

    // @Override
    // protected void onDraw(Canvas canvas) {
    // super.onDraw(canvas);
    // }

    @Override
    public AccessibilityNodeProvider getAccessibilityNodeProvider() {
        AccessibilityNodeProvider provider = mProvider.getAccessibilityNodeProvider();
        return provider == null ? super.getAccessibilityNodeProvider() : provider;
    }

    @Override
    public boolean performAccessibilityAction(int action, Bundle arguments) {
        if (mProvider != null) {
            return mProvider.performAccessibilityAction(action, arguments);
        } else {
            return super.performAccessibilityAction(action, arguments);
        }
    }

    @Override
    public boolean onDragEvent(DragEvent event) {
        if (mProvider != null) {
            return mProvider.onDragEvent(event);
        } else {
            return super.onDragEvent(event);
        }

    }

    public static void setRemoteDebuggingEnabled(boolean enable) {
        if (gBvDevToolsServer == null) {
            if (!enable)
                return;
            gBvDevToolsServer = new BvDevToolsServer();
        }
        gBvDevToolsServer.setRemoteDebuggingEnabled(enable);
        if (!enable) {
            gBvDevToolsServer.destroy();
            gBvDevToolsServer = null;
        }
    }

    // public void logCommandLineForDebugging() {
    // BvContents.logCommandLineForDebugging();
    // }

    public class InternalAccess {

        public boolean super_onKeyUp(int keyCode, KeyEvent event) {
            return BisonView.super.onKeyUp(keyCode, event);
        }

        public boolean super_dispatchKeyEvent(KeyEvent event) {
            return BisonView.super.dispatchKeyEvent(event);
        }

        public boolean super_onGenericMotionEvent(MotionEvent event) {
            return BisonView.super.onGenericMotionEvent(event);
        }

        public void super_onConfigurationChanged(Configuration newConfig) {
            BisonView.super.onConfigurationChanged(newConfig);
        }

        public void super_scrollTo(int scrollX, int scrollY) {
            // We're intentionally not calling super.scrollTo here to make testing easier.
            BisonView.this.scrollTo(scrollX, scrollY);
        }

        public void overScrollBy(int deltaX, int deltaY, int scrollX, int scrollY, int scrollRangeX, int scrollRangeY,
                int maxOverScrollX, int maxOverScrollY, boolean isTouchEvent) {
            // We're intentionally not calling super.scrollTo here to make testing easier.
            BisonView.this.overScrollBy(deltaX, deltaY, scrollX, scrollY, scrollRangeX, scrollRangeY, maxOverScrollX,
                    maxOverScrollY, isTouchEvent);
        }

        public void onScrollChanged(int l, int t, int oldl, int oldt) {
            BisonView.super.onScrollChanged(l, t, oldl, oldt);
        }

        public void setMeasuredDimension(int measuredWidth, int measuredHeight) {
            BisonView.super.setMeasuredDimension(measuredWidth, measuredHeight);
        }

        public int super_getScrollBarStyle() {
            return BisonView.super.getScrollBarStyle();
        }

        public void super_startActivityForResult(Intent intent, int requestCode) {
        }

        public boolean super_requestFocus(int direction, Rect previouslyFocusedRect) {
            return BisonView.super.requestFocus(direction, previouslyFocusedRect);
        }

        public boolean super_performLongClick() {
            return BisonView.super.performLongClick();
        }

        public void super_setLayoutParams(ViewGroup.LayoutParams params) {
            BisonView.super.setLayoutParams(params);
        }

        public boolean super_performAccessibilityAction(int action, Bundle arguments) {
            return BisonView.super.performAccessibilityAction(action, arguments);
        }

    }

    public interface FindListener {
        void onFindResultReceived(int activeMatchOrdinal, int numberOfMatches, boolean isDoneCounting);
    }

    public static abstract class VisualStateCallback {
        /**
         * Invoked when the visual state is ready to be drawn in the next
         * {@link #onDraw}.
         *
         * @param requestId The identifier passed to {@link #postVisualStateCallback}
         *                  when this callback was posted.
         */
        public abstract void onComplete(long requestId);
    }

    public static class HitTestResult {
        /**
         * Default HitTestResult, where the target is unknown.
         */
        public static final int UNKNOWN_TYPE = 0;
        /**
         * @deprecated This type is no longer used.
         */
        @Deprecated
        public static final int ANCHOR_TYPE = 1;
        /**
         * HitTestResult for hitting a phone number.
         */
        public static final int PHONE_TYPE = 2;
        /**
         * HitTestResult for hitting a map address.
         */
        public static final int GEO_TYPE = 3;
        /**
         * HitTestResult for hitting an email address.
         */
        public static final int EMAIL_TYPE = 4;
        /**
         * HitTestResult for hitting an HTML::img tag.
         */
        public static final int IMAGE_TYPE = 5;
        /**
         * @deprecated This type is no longer used.
         */
        @Deprecated
        public static final int IMAGE_ANCHOR_TYPE = 6;
        /**
         * HitTestResult for hitting a HTML::a tag with src=http.
         */
        public static final int SRC_ANCHOR_TYPE = 7;
        /**
         * HitTestResult for hitting a HTML::a tag with src=http + HTML::img.
         */
        public static final int SRC_IMAGE_ANCHOR_TYPE = 8;
        /**
         * HitTestResult for hitting an edit text area.
         */
        public static final int EDIT_TEXT_TYPE = 9;

        private int mType;
        private String mExtra;

        @RestrictTo(LIBRARY_GROUP)
        public HitTestResult() {
            mType = UNKNOWN_TYPE;
        }

        @RestrictTo(LIBRARY_GROUP)
        public void setType(int type) {
            mType = type;
        }

        @RestrictTo(LIBRARY_GROUP)
        public void setExtra(String extra) {
            mExtra = extra;
        }

        /**
         * Gets the type of the hit test result. See the XXX_TYPE constants defined in
         * this class.
         *
         * @return the type of the hit test result
         */
        public int getType() {
            return mType;
        }

        /**
         * Gets additional type-dependant information about the result. See
         * {@link BisonView#getHitTestResult()} for details. May either be {@code null}
         * or contain extra information about this result.
         *
         * @return additional type-dependant information about the result
         */
        @Nullable
        public String getExtra() {
            return mExtra;
        }
    }

    public interface DownloadListener {

        /**
         * Notify the host application that a file should be downloaded
         *
         * @param url                The full url to the content that should be
         *                           downloaded
         * @param userAgent          the user agent to be used for the download.
         * @param contentDisposition Content-disposition http header, if present.
         * @param mimetype           The mimetype of the content reported by the server
         * @param contentLength      The file size reported by the server
         */
        public void onDownloadStart(String url, String userAgent, String contentDisposition, String mimetype,
                long contentLength);

    }

    public interface PictureListener {
        void onNewPicture(BisonView view, Picture picture);
    }

}
