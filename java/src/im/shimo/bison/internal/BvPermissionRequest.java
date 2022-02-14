
package im.shimo.bison.internal;

import android.net.Uri;
import im.shimo.bison.CleanupReference;

import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;


/**
 * This class wraps permission request in Chromium side, and can only be created
 * by native side.
 */
@JNINamespace("bison")
public class BvPermissionRequest {
    private final Uri mOrigin;
    private final long mResources;
    private boolean mProcessed;

    // BvPermissionRequest native instance.
    private long mNativeBvPermissionRequest;

    // Responsible for deleting native peer.
    private CleanupReference mCleanupReference;

    // This should be same as corresponding definition in PermissionRequest.
    // We duplicate definition because it is used in Android L and afterwards, but is only
    // defined in M.
    // TODO(michaelbai) : Replace "android.webkit.resource.MIDI_SYSEX" with
    // PermissionRequest.RESOURCE_MIDI_SYSEX once Android M SDK is used.
    public static final String RESOURCE_MIDI_SYSEX = "android.webkit.resource.MIDI_SYSEX";

    private static final class DestroyRunnable implements Runnable {
        private final long mNativeBvPermissionRequest;

        private DestroyRunnable(long nativeBvPermissionRequest) {
            mNativeBvPermissionRequest = nativeBvPermissionRequest;
        }
        @Override
        public void run() {
            BvPermissionRequestJni.get().destroy(mNativeBvPermissionRequest);
        }
    }

    @CalledByNative
    private static BvPermissionRequest create(long nativeBvPermissionRequest, String url,
                                                 long resources) {
        if (nativeBvPermissionRequest == 0) return null;
        Uri origin = Uri.parse(url);
        return new BvPermissionRequest(nativeBvPermissionRequest, origin, resources);
    }

    private BvPermissionRequest(long nativeBvPermissionRequest, Uri origin,
                                   long resources) {
        mNativeBvPermissionRequest = nativeBvPermissionRequest;
        mOrigin = origin;
        mResources = resources;
        mCleanupReference =
                new CleanupReference(this, new DestroyRunnable(mNativeBvPermissionRequest));
    }

    public Uri getOrigin() {
        return mOrigin;
    }

    public long getResources() {
        return mResources;
    }

    public void grant() {
        validate();
        if (mNativeBvPermissionRequest != 0) {
            BvPermissionRequestJni.get().onAccept(
                    mNativeBvPermissionRequest, BvPermissionRequest.this, true);
            destroyNative();
        }
        mProcessed = true;
    }

    public void deny() {
        validate();
        if (mNativeBvPermissionRequest != 0) {
            BvPermissionRequestJni.get().onAccept(
                    mNativeBvPermissionRequest, BvPermissionRequest.this, false);
            destroyNative();
        }
        mProcessed = true;
    }

    @CalledByNative
    private void destroyNative() {
        mCleanupReference.cleanupNow();
        mCleanupReference = null;
        mNativeBvPermissionRequest = 0;
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
        void onAccept(long nativeBvPermissionRequest, BvPermissionRequest caller, boolean allowed);
        void destroy(long nativeBvPermissionRequest);
    }
}
