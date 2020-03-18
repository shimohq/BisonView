// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison;

import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.accessibility.AccessibilityNodeProvider;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.FrameLayout;

/**
 * A view that is used to render the web contents in fullscreen mode, ie.
 * html controls and subtitles, over the {@link ContentVideoView}.
 */
public class FullScreenView extends FrameLayout {

    private BisonViewMethods mBisonViewMethods;
    private final BisonContents mBisonContents;
    private InternalAccessAdapter mInternalAccessAdapter;

    public FullScreenView(Context context, BisonViewMethods bisonViewMethods, BisonContents bisonContents,
                          int initialWidth, int initialHeight) {
        super(context);
        setRight(initialWidth);
        setBottom(initialHeight);
        setBisonViewMethods(bisonViewMethods);
        mBisonContents = bisonContents;
        mInternalAccessAdapter = new InternalAccessAdapter();
    }

    public InternalAccessAdapter getInternalAccessAdapter() {
        return mInternalAccessAdapter;
    }

    public void setBisonViewMethods(BisonViewMethods awViewMethods) {
        mBisonViewMethods = awViewMethods;
    }

    @Override
    public void onDraw(final Canvas canvas) {
        mBisonViewMethods.onDraw(canvas);
    }

    @Override
    public void onMeasure(final int widthMeasureSpec, final int heightMeasureSpec) {
        mBisonViewMethods.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    @Override
    public boolean requestFocus(final int direction, final Rect previouslyFocusedRect) {
        mBisonViewMethods.requestFocus();
        return super.requestFocus(direction, previouslyFocusedRect);
    }

    @Override
    public void setLayerType(int layerType, Paint paint) {
        super.setLayerType(layerType, paint);
        mBisonViewMethods.setLayerType(layerType, paint);
    }

    @Override
    public InputConnection onCreateInputConnection(final EditorInfo outAttrs) {
        return mBisonViewMethods.onCreateInputConnection(outAttrs);
    }

    @Override
    public boolean onKeyUp(final int keyCode, final KeyEvent event) {
        return mBisonViewMethods.onKeyUp(keyCode, event);
    }

    @Override
    public boolean dispatchKeyEvent(final KeyEvent event) {
        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK
                && event.getAction() == KeyEvent.ACTION_UP
                && mBisonContents.isFullScreen()) {
            mBisonContents.requestExitFullscreen();
            return true;
        }
        return mBisonViewMethods.dispatchKeyEvent(event);
    }

    @Override
    public boolean onTouchEvent(final MotionEvent event) {
        return mBisonViewMethods.onTouchEvent(event);
    }

    @Override
    public boolean onHoverEvent(final MotionEvent event) {
        return mBisonViewMethods.onHoverEvent(event);
    }

    @Override
    public boolean onGenericMotionEvent(final MotionEvent event) {
        return mBisonViewMethods.onGenericMotionEvent(event);
    }

    @Override
    public void onConfigurationChanged(final Configuration newConfig) {
        mBisonViewMethods.onConfigurationChanged(newConfig);
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        mBisonViewMethods.onAttachedToWindow();
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        mBisonViewMethods.onDetachedFromWindow();
    }

    @Override
    public void onWindowFocusChanged(final boolean hasWindowFocus) {
        super.onWindowFocusChanged(hasWindowFocus);
        mBisonViewMethods.onWindowFocusChanged(hasWindowFocus);
    }

    @Override
    public void onFocusChanged(final boolean focused, final int direction,
            final Rect previouslyFocusedRect) {
        super.onFocusChanged(focused, direction, previouslyFocusedRect);
        mBisonViewMethods.onFocusChanged(
                focused, direction, previouslyFocusedRect);
    }

    @Override
    public void onSizeChanged(final int w, final int h, final int ow, final int oh) {
        super.onSizeChanged(w, h, ow, oh);
        // Null check for setting initial size before mBisonViewMethods is set.
        if (mBisonViewMethods != null) {
            mBisonViewMethods.onSizeChanged(w, h, ow, oh);
        }
    }

    @Override
    protected void onVisibilityChanged(View changedView, int visibility) {
        super.onVisibilityChanged(changedView, visibility);
        mBisonViewMethods.onVisibilityChanged(changedView, visibility);
    }

    @Override
    public void onWindowVisibilityChanged(final int visibility) {
        super.onWindowVisibilityChanged(visibility);
        mBisonViewMethods.onWindowVisibilityChanged(visibility);
    }

    @Override
    public void onOverScrolled(int scrollX, int scrollY, boolean clampedX, boolean clampedY) {
        mBisonViewMethods.onContainerViewOverScrolled(scrollX, scrollY, clampedX, clampedY);
    }

    @Override
    public void onScrollChanged(int l, int t, int oldl, int oldt) {
        super.onScrollChanged(l, t, oldl, oldt);
        mBisonViewMethods.onContainerViewScrollChanged(l, t, oldl, oldt);
    }

    @Override
    public int computeHorizontalScrollRange() {
        return mBisonViewMethods.computeHorizontalScrollRange();
    }

    @Override
    public int computeHorizontalScrollOffset() {
        return mBisonViewMethods.computeHorizontalScrollOffset();
    }

    @Override
    public int computeVerticalScrollRange() {
        return mBisonViewMethods.computeVerticalScrollRange();
    }

    @Override
    public int computeVerticalScrollOffset() {
        return mBisonViewMethods.computeVerticalScrollOffset();
    }

    @Override
    public int computeVerticalScrollExtent() {
        return mBisonViewMethods.computeVerticalScrollExtent();
    }

    @Override
    public void computeScroll() {
        mBisonViewMethods.computeScroll();
    }

    @Override
    public AccessibilityNodeProvider getAccessibilityNodeProvider() {
        return mBisonViewMethods.getAccessibilityNodeProvider();
    }

    @Override
    public boolean performAccessibilityAction(final int action, final Bundle arguments) {
        return mBisonViewMethods.performAccessibilityAction(action, arguments);
    }

    // BisonContents.InternalAccessDelegate implementation --------------------------------------
    private class InternalAccessAdapter implements BisonContents.InternalAccessDelegate {

        @Override
        public boolean super_onKeyUp(int keyCode, KeyEvent event) {
            return FullScreenView.super.onKeyUp(keyCode, event);
        }

        @Override
        public boolean super_dispatchKeyEvent(KeyEvent event) {
            return FullScreenView.super.dispatchKeyEvent(event);
        }

        @Override
        public boolean super_onGenericMotionEvent(MotionEvent event) {
            return FullScreenView.super.onGenericMotionEvent(event);
        }

        @Override
        public void super_onConfigurationChanged(Configuration newConfig) {
            // Intentional no-op
        }

        @Override
        public int super_getScrollBarStyle() {
            return FullScreenView.super.getScrollBarStyle();
        }

        @Override
        public void super_startActivityForResult(Intent intent, int requestCode) {
            throw new RuntimeException(
                    "FullScreenView InternalAccessAdapter shouldn't call startActivityForResult. "
                    + "See BisonContents#startActivityForResult");
        }

        @Override
        public void onScrollChanged(int lPix, int tPix, int oldlPix, int oldtPix) {
            FullScreenView.this.onScrollChanged(lPix, tPix, oldlPix, oldtPix);
        }

        @Override
        public void overScrollBy(int deltaX, int deltaY, int scrollX, int scrollY,
                int scrollRangeX, int scrollRangeY, int maxOverScrollX,
                int maxOverScrollY, boolean isTouchEvent) {
            FullScreenView.this.overScrollBy(deltaX, deltaY, scrollX, scrollY, scrollRangeX,
                    scrollRangeY, maxOverScrollX, maxOverScrollY, isTouchEvent);
        }

        @Override
        public void super_scrollTo(int scrollX, int scrollY) {
            FullScreenView.super.scrollTo(scrollX, scrollY);
        }

        @Override
        public void setMeasuredDimension(int measuredWidth, int measuredHeight) {
            FullScreenView.this.setMeasuredDimension(measuredWidth, measuredHeight);
        }
    }
}
