// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

/**
 * See {@link android.webkit.HttpAuthHandler}.
 */
@JNINamespace("bison")
public class BisonHttpAuthHandler {

    private long mNativeBisonHttpAuthHandler;
    private final boolean mFirstAttempt;

    public void proceed(String username, String password) {
        if (mNativeBisonHttpAuthHandler != 0) {
            BisonHttpAuthHandlerJni.get().proceed(
                    mNativeBisonHttpAuthHandler, BisonHttpAuthHandler.this, username, password);
            mNativeBisonHttpAuthHandler = 0;
        }
    }

    public void cancel() {
        if (mNativeBisonHttpAuthHandler != 0) {
            BisonHttpAuthHandlerJni.get().cancel(mNativeBisonHttpAuthHandler, BisonHttpAuthHandler.this);
            mNativeBisonHttpAuthHandler = 0;
        }
    }

    public boolean isFirstAttempt() {
        return mFirstAttempt;
    }

    @CalledByNative
    public static BisonHttpAuthHandler create(long nativeBisonAuthHandler, boolean firstAttempt) {
        return new BisonHttpAuthHandler(nativeBisonAuthHandler, firstAttempt);
    }

    private BisonHttpAuthHandler(long nativeBisonHttpAuthHandler, boolean firstAttempt) {
        mNativeBisonHttpAuthHandler = nativeBisonHttpAuthHandler;
        mFirstAttempt = firstAttempt;
    }

    @CalledByNative
    void handlerDestroyed() {
        mNativeBisonHttpAuthHandler = 0;
    }

    @NativeMethods
    interface Natives {
        void proceed(long nativeBisonHttpAuthHandler, BisonHttpAuthHandler caller, String username,
                     String password);
        void cancel(long nativeBisonHttpAuthHandler, BisonHttpAuthHandler caller);
    }
}
