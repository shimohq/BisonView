// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.permission;

import android.net.Uri;

import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

import im.shimo.bison.CleanupReference;

/**
 * This class wraps permission request in Chromium side, and can only be created
 * by native side.
 */
@JNINamespace("bison")
public class BisonPermissionRequest {
    private final Uri mOrigin;
    private final long mResources;
    private boolean mProcessed;

    // BisonPermissionRequest native instance.
    private long mNativeBisonPermissionRequest;

    // Responsible for deleting native peer.
    private CleanupReference mCleanupReference;

    // This should be same as corresponding definition in PermissionRequest.
    // We duplicate definition because it is used in Android L and afterwards, but is only
    // defined in M.
    // TODO(michaelbai) : Replace "android.webkit.resource.MIDI_SYSEX" with
    // PermissionRequest.RESOURCE_MIDI_SYSEX once Android M SDK is used.
    public static final String RESOURCE_MIDI_SYSEX = "android.webkit.resource.MIDI_SYSEX";

    private static final class DestroyRunnable implements Runnable {
        private final long mNativeBisonPermissionRequest;

        private DestroyRunnable(long nativeBisonPermissionRequest) {
            mNativeBisonPermissionRequest = nativeBisonPermissionRequest;
        }
        @Override
        public void run() {
            BisonPermissionRequestJni.get().destroy(mNativeBisonPermissionRequest);
        }
    }

    @CalledByNative
    private static BisonPermissionRequest create(long nativeBisonPermissionRequest, String url,
                                                 long resources) {
        if (nativeBisonPermissionRequest == 0) return null;
        Uri origin = Uri.parse(url);
        return new BisonPermissionRequest(nativeBisonPermissionRequest, origin, resources);
    }

    private BisonPermissionRequest(long nativeBisonPermissionRequest, Uri origin,
                                   long resources) {
        mNativeBisonPermissionRequest = nativeBisonPermissionRequest;
        mOrigin = origin;
        mResources = resources;
        mCleanupReference =
                new CleanupReference(this, new DestroyRunnable(mNativeBisonPermissionRequest));
    }

    public Uri getOrigin() {
        return mOrigin;
    }

    public long getResources() {
        return mResources;
    }

    public void grant() {
        validate();
        if (mNativeBisonPermissionRequest != 0) {
            BisonPermissionRequestJni.get().onAccept(
                    mNativeBisonPermissionRequest, BisonPermissionRequest.this, true);
            destroyNative();
        }
        mProcessed = true;
    }

    public void deny() {
        validate();
        if (mNativeBisonPermissionRequest != 0) {
            BisonPermissionRequestJni.get().onAccept(
                    mNativeBisonPermissionRequest, BisonPermissionRequest.this, false);
            destroyNative();
        }
        mProcessed = true;
    }

    @CalledByNative
    private void destroyNative() {
        mCleanupReference.cleanupNow();
        mCleanupReference = null;
        mNativeBisonPermissionRequest = 0;
    }

    private void validate() {
        if (!ThreadUtils.runningOnUiThread()) {
            throw new IllegalStateException(
                    "Either grant() or deny() should be called on UI thread");
        }

        if (mProcessed) {
            throw new IllegalStateException("Either grant() or deny() has been already called.");
        }
    }

    @NativeMethods
    interface Natives {
        void onAccept(long nativeBisonPermissionRequest, BisonPermissionRequest caller, boolean allowed);
        void destroy(long nativeBisonPermissionRequest);
    }
}
