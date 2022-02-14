
package im.shimo.bison;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;

import im.shimo.bison.internal.BvContents;
import im.shimo.bison.internal.BvGeolocationPermissions;

import java.lang.ref.WeakReference;

/**
 * This class implements BisonGeolocationPermissions.Callback, and will be sent to
 * WebView applications through WebChromeClient.onGeolocationPermissionsShowPrompt().
 */
public class BvGeolocationCallback implements BvGeolocationPermissions.Callback {
    private static final String TAG = "Geolocation";

    private CleanupRunable mCleanupRunable;
    private CleanupReference mCleanupReference;

    private static class CleanupRunable implements Runnable {
        private WeakReference<BvContents> mBisonContents;
        private boolean mAllow;
        private boolean mRetain;
        private String mOrigin;

        public CleanupRunable(BvContents bvContents, String origin) {
            mBisonContents = new WeakReference<BvContents>(bvContents);
            mOrigin = origin;
        }

        @Override
        public void run() {
            assert ThreadUtils.runningOnUiThread();
            BvContents bvContents = mBisonContents.get();
            if (bvContents == null) return;
            if (mRetain) {
                if (mAllow) {
                    bvContents.getGeolocationPermissions().allow(mOrigin);
                } else {
                    bvContents.getGeolocationPermissions().deny(mOrigin);
                }
            }
            bvContents.invokeGeolocationCallback(mAllow, mOrigin);
        }

        public void setResponse(String origin, boolean allow, boolean retain) {
            mOrigin = origin;
            mAllow = allow;
            mRetain = retain;
        }
    }

    public BvGeolocationCallback(String origin, BvContents bvContents) {
        mCleanupRunable = new CleanupRunable(bvContents, origin);
        mCleanupReference = new CleanupReference(this, mCleanupRunable);
    }

    @Override
    public void invoke(String origin, boolean allow, boolean retain) {
        if (mCleanupRunable == null || mCleanupReference == null) {
            Log.w(TAG, "Response for this geolocation request has been received."
                    + " Ignoring subsequent responses");
            return;
        }
        mCleanupRunable.setResponse(origin, allow, retain);
        mCleanupReference.cleanupNow();
        mCleanupReference = null;
        mCleanupRunable = null;
    }
}
