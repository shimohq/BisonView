// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.gfx;

import android.graphics.Canvas;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

/**
 * Implementation of draw_fn.h.
 */
@JNINamespace("bison")
public class BisonDrawFnImpl implements BisonFunctor {
    private long mNativeBisonDrawFnImpl;
    private final DrawFnAccess mAccess;
    private final int mHandle;

    /** Interface for inserting functor into canvas */
    public interface DrawFnAccess { void drawWebViewFunctor(Canvas canvas, int functor); }

    public BisonDrawFnImpl(DrawFnAccess access) {
        mAccess = access;
        mNativeBisonDrawFnImpl = BisonDrawFnImplJni.get().create();
        mHandle = BisonDrawFnImplJni.get().getFunctorHandle(mNativeBisonDrawFnImpl, BisonDrawFnImpl.this);
    }

    @Override
    public void destroy() {
        assert mNativeBisonDrawFnImpl != 0;
        BisonDrawFnImplJni.get().releaseHandle(mNativeBisonDrawFnImpl, BisonDrawFnImpl.this);
        // Native side is free to destroy itself after ReleaseHandle.
        mNativeBisonDrawFnImpl = 0;
    }

    public static void setDrawFnFunctionTable(long functionTablePointer) {
        BisonDrawFnImplJni.get().setDrawFnFunctionTable(functionTablePointer);
    }

    @Override
    public long getNativeCompositorFrameConsumer() {
        assert mNativeBisonDrawFnImpl != 0;
        return BisonDrawFnImplJni.get().getCompositorFrameConsumer(
                mNativeBisonDrawFnImpl, BisonDrawFnImpl.this);
    }

    @Override
    public boolean requestDraw(Canvas canvas) {
        assert mNativeBisonDrawFnImpl != 0;
        mAccess.drawWebViewFunctor(canvas, mHandle);
        return true;
    }

    @Override
    public void trimMemory() {}

    @NativeMethods
    interface Natives {
        int getFunctorHandle(long nativeBisonDrawFnImpl, BisonDrawFnImpl caller);
        long getCompositorFrameConsumer(long nativeBisonDrawFnImpl, BisonDrawFnImpl caller);
        void releaseHandle(long nativeBisonDrawFnImpl, BisonDrawFnImpl caller);
        void setDrawFnFunctionTable(long functionTablePointer);
        long create();
    }
}
