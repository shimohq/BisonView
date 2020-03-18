// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.gfx;

import android.graphics.Canvas;
import android.view.ViewGroup;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

import im.shimo.bison.BisonContents;

/**
 * Manages state associated with the Android render thread and the draw functor
 * that the WebView uses to render its contents. BisonGLFunctor is responsible for
 * managing the lifetime of native RenderThreadManager and HardwareRenderer,
 * ensuring that they continue to exist while the functor remains attached to
 * the render node hierarchy.
 */
@JNINamespace("bison")
public class BisonGLFunctor implements BisonFunctor {
    private final long mNativeBisonGLFunctor;
    private final BisonContents.NativeDrawGLFunctor mNativeDrawGLFunctor;
    private final ViewGroup mContainerView;
    private final Runnable mFunctorReleasedCallback;
    // Counts outstanding requestDrawGL calls as well as window attach count.
    private int mRefCount;

    public BisonGLFunctor(
            BisonContents.NativeDrawFunctorFactory nativeDrawFunctorFactory, ViewGroup containerView) {
        mNativeBisonGLFunctor = BisonGLFunctorJni.get().create(this);
        mNativeDrawGLFunctor = nativeDrawFunctorFactory.createGLFunctor(mNativeBisonGLFunctor);
        mContainerView = containerView;
        if (mNativeDrawGLFunctor.supportsDrawGLFunctorReleasedCallback()) {
            mFunctorReleasedCallback = () -> removeReference();
        } else {
            mFunctorReleasedCallback = null;
        }
        addReference();
    }

    @Override
    public void destroy() {
        assert mRefCount > 0;
        BisonGLFunctorJni.get().removeFromCompositorFrameProducer(
                mNativeBisonGLFunctor, BisonGLFunctor.this);
        removeReference();
    }

    public static long getBisonDrawGLFunction() {
        return BisonGLFunctorJni.get().getBisonDrawGLFunction();
    }

    @Override
    public long getNativeCompositorFrameConsumer() {
        assert mRefCount > 0;
        return BisonGLFunctorJni.get().getCompositorFrameConsumer(
                mNativeBisonGLFunctor, BisonGLFunctor.this);
    }

    @Override
    public boolean requestDraw(Canvas canvas) {
        assert mRefCount > 0;
        boolean success = mNativeDrawGLFunctor.requestDrawGL(canvas, mFunctorReleasedCallback);
        if (success && mFunctorReleasedCallback != null) {
            addReference();
        }
        return success;
    }

    private void addReference() {
        ++mRefCount;
    }

    private void removeReference() {
        assert mRefCount > 0;
        if (--mRefCount == 0) {
            // When |mRefCount| decreases to zero, the functor is neither attached to a view, nor
            // referenced from the render tree, and so it is safe to delete the HardwareRenderer
            // instance to free up resources because the current state will not be drawn again.
            BisonGLFunctorJni.get().deleteHardwareRenderer(mNativeBisonGLFunctor, BisonGLFunctor.this);
            mNativeDrawGLFunctor.destroy();
            BisonGLFunctorJni.get().destroy(mNativeBisonGLFunctor);
        }
    }

    @CalledByNative
    private boolean requestInvokeGL(boolean waitForCompletion) {
        return mNativeDrawGLFunctor.requestInvokeGL(mContainerView, waitForCompletion);
    }

    @CalledByNative
    private void detachFunctorFromView() {
        mNativeDrawGLFunctor.detach(mContainerView);
        mContainerView.invalidate();
    }

    @Override
    public void trimMemory() {
        assert mRefCount > 0;
        BisonGLFunctorJni.get().deleteHardwareRenderer(mNativeBisonGLFunctor, BisonGLFunctor.this);
    }

    /**
     * Intended for test code.
     * @return the number of native instances of this class.
     */
    @VisibleForTesting
    public static int getNativeInstanceCount() {
        return BisonGLFunctorJni.get().getNativeInstanceCount();
    }

    @NativeMethods
    interface Natives {
        void deleteHardwareRenderer(long nativeBisonGLFunctor, BisonGLFunctor caller);
        void removeFromCompositorFrameProducer(long nativeBisonGLFunctor, BisonGLFunctor caller);
        long getCompositorFrameConsumer(long nativeBisonGLFunctor, BisonGLFunctor caller);
        long getBisonDrawGLFunction();
        void destroy(long nativeBisonGLFunctor);
        long create(BisonGLFunctor javaProxy);
        int getNativeInstanceCount();
    }
}
