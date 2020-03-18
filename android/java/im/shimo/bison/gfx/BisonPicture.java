// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.gfx;

import android.graphics.Canvas;
import android.graphics.Picture;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

import java.io.OutputStream;

import im.shimo.bison.CleanupReference;

/**
 * A simple wrapper around a SkPicture, that allows final rendering to be performed using the
 * chromium skia library.
 */
@JNINamespace("bison")
public class BisonPicture extends Picture {
    private long mNativeBisonPicture;

    // There is no explicit destroy method on Picture base-class, so cleanup is always
    // handled via the CleanupReference.
    private static final class DestroyRunnable implements Runnable {
        private long mNativeBisonPicture;
        private DestroyRunnable(long nativeBisonPicture) {
            mNativeBisonPicture = nativeBisonPicture;
        }
        @Override
        public void run() {
            BisonPictureJni.get().destroy(mNativeBisonPicture);
        }
    }

    private CleanupReference mCleanupReference;

    /**
     * @param nativeBisonPicture is an instance of the BisonPicture native class. Ownership is
     *                        taken by this java instance.
     */
    public BisonPicture(long nativeBisonPicture) {
        mNativeBisonPicture = nativeBisonPicture;
        mCleanupReference = new CleanupReference(this, new DestroyRunnable(nativeBisonPicture));
    }

    @Override
    public Canvas beginRecording(int width, int height) {
        unsupportedOperation();
        return null;
    }

    @Override
    public void endRecording() {
        // Intentional no-op. The native picture ended recording prior to java c'tor call.
    }

    @Override
    public int getWidth() {
        return BisonPictureJni.get().getWidth(mNativeBisonPicture, BisonPicture.this);
    }

    @Override
    public int getHeight() {
        return BisonPictureJni.get().getHeight(mNativeBisonPicture, BisonPicture.this);
    }

    @Override
    public void draw(Canvas canvas) {
        BisonPictureJni.get().draw(mNativeBisonPicture, BisonPicture.this, canvas);
    }

    @SuppressWarnings("deprecation")
    public void writeToStream(OutputStream stream) {
        unsupportedOperation();
    }

    private void unsupportedOperation() {
        throw new IllegalStateException("Unsupported in BisonPicture");
    }

    @NativeMethods
    interface Natives {
        void destroy(long nativeBisonPicture);
        int getWidth(long nativeBisonPicture, BisonPicture caller);
        int getHeight(long nativeBisonPicture, BisonPicture caller);
        void draw(long nativeBisonPicture, BisonPicture caller, Canvas canvas);
    }
}
