// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.core;

import android.content.res.Configuration;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Bundle;
import android.view.DragEvent;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.accessibility.AccessibilityNodeProvider;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

import im.shimo.bison.core.BisonContents.InternalAccessDelegate;

/**
 * No-op implementation of {@link BisonViewMethods} that follows the null object pattern.
 * This {@link NullBisonViewMethods} is hooked up to the WebView in fullscreen mode, and
 * to the {@link FullScreenView} in embedded mode, but not to both at the same time.
 */
class NullBisonViewMethods implements BisonViewMethods {
    private BisonContents mBisonContents;
    private InternalAccessDelegate mInternalAccessAdapter;
    private View mContainerView;

    public NullBisonViewMethods(
            BisonContents awContents, InternalAccessDelegate internalAccessAdapter,
            View containerView) {
        mBisonContents = awContents;
        mInternalAccessAdapter = internalAccessAdapter;
        mContainerView = containerView;
    }

    @Override
    public void onDraw(Canvas canvas) {
        canvas.drawColor(mBisonContents.getEffectiveBackgroundColor());
    }

    @Override
    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        // When the containerView is using the NullBisonViewMethods then it is not
        // attached to the BisonContents. As such, we don't have any contents to measure
        // and using the last measured dimension is the best we can do.
        mInternalAccessAdapter.setMeasuredDimension(
                mContainerView.getMeasuredWidth(), mContainerView.getMeasuredHeight());
    }

    @Override
    public void requestFocus() {
        // Intentional no-op.
    }

    @Override
    public void setLayerType(int layerType, Paint paint) {
        // Intentional no-op.
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        return null; // Intentional no-op.
    }

    @Override
    public boolean onDragEvent(DragEvent event) {
        return false; // Intentional no-op.
    }
    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        return false; // Intentional no-op.
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        return false; // Intentional no-op.
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        return false; // Intentional no-op.
    }

    @Override
    public boolean onHoverEvent(MotionEvent event) {
        return false; // Intentional no-op.
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        return false; // Intentional no-op.
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        // Intentional no-op.
    }

    @Override
    public void onAttachedToWindow() {
        // Intentional no-op.
    }

    @Override
    public void onDetachedFromWindow() {
        // Intentional no-op.
    }

    @Override
    public void onWindowFocusChanged(boolean hasWindowFocus) {
        // Intentional no-op.
    }

    @Override
    public void onFocusChanged(boolean focused, int direction, Rect previouslyFocusedRect) {
        // Intentional no-op.
    }

    @Override
    public void onSizeChanged(int w, int h, int ow, int oh) {
        // Intentional no-op.
    }

    @Override
    public void onVisibilityChanged(View changedView, int visibility) {
        // Intentional no-op.
    }

    @Override
    public void onWindowVisibilityChanged(int visibility) {
        // Intentional no-op.
    }

    @Override
    public void onContainerViewScrollChanged(int l, int t, int oldl, int oldt) {
        // Intentional no-op.
    }

    @Override
    public void onContainerViewOverScrolled(int scrollX, int scrollY, boolean clampedX,
            boolean clampedY) {
        // Intentional no-op.
    }

    @Override
    public int computeHorizontalScrollRange() {
        return 0;
    }

    @Override
    public int computeHorizontalScrollOffset() {
        return 0;
    }

    @Override
    public int computeVerticalScrollRange() {
        return 0;
    }

    @Override
    public int computeVerticalScrollOffset() {
        return 0;
    }

    @Override
    public int computeVerticalScrollExtent() {
        return 0;
    }

    @Override
    public void computeScroll() {
        // Intentional no-op.
    }

    @Override
    public boolean onCheckIsTextEditor() {
        return false;
    }

    @Override
    public AccessibilityNodeProvider getAccessibilityNodeProvider() {
        return null;
    }

    @Override
    public boolean performAccessibilityAction(final int action, final Bundle arguments) {
        return false;
    }
}