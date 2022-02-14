// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.internal;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

/**
 * See {@link android.webkit.HttpAuthHandler}.
 */
@JNINamespace("bison")
public class BvHttpAuthHandler {

    private long mNativeBvHttpAuthHandler;
    private final boolean mFirstAttempt;

    public void proceed(String username, String password) {
        if (mNativeBvHttpAuthHandler != 0) {
            BvHttpAuthHandlerJni.get().proceed(
                    mNativeBvHttpAuthHandler, BvHttpAuthHandler.this, username, password);
            mNativeBvHttpAuthHandler = 0;
        }
    }

    public void cancel() {
        if (mNativeBvHttpAuthHandler != 0) {
            BvHttpAuthHandlerJni.get().cancel(mNativeBvHttpAuthHandler, BvHttpAuthHandler.this);
            mNativeBvHttpAuthHandler = 0;
        }
    }

    public boolean isFirstAttempt() {
        return mFirstAttempt;
    }

    @CalledByNative
    public static BvHttpAuthHandler create(long nativeBvAuthHandler, boolean firstAttempt) {
        return new BvHttpAuthHandler(nativeBvAuthHandler, firstAttempt);
    }

    private BvHttpAuthHandler(long nativeBvHttpAuthHandler, boolean firstAttempt) {
        mNativeBvHttpAuthHandler = nativeBvHttpAuthHandler;
        mFirstAttempt = firstAttempt;
    }

    @CalledByNative
    void handlerDestroyed() {
        mNativeBvHttpAuthHandler = 0;
    }

    @NativeMethods
    interface Natives {
        void proceed(long nativeBvHttpAuthHandler, BvHttpAuthHandler caller, String username,
                String password);
        void cancel(long nativeBvHttpAuthHandler, BvHttpAuthHandler caller);
    }
}
