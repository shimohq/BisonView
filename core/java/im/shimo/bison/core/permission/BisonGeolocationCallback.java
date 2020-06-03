// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.core.permission;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;

import java.lang.ref.WeakReference;

import im.shimo.bison.core.BisonContents;
import im.shimo.bison.core.BisonGeolocationPermissions;



import im.shimo.bison.core.BisonContents;
import im.shimo.bison.core.CleanupReference;

/**
 * This class implements BisonGeolocationPermissions.Callback, and will be sent to
 * WebView applications through WebChromeClient.onGeolocationPermissionsShowPrompt().
 */
public class BisonGeolocationCallback implements BisonGeolocationPermissions.Callback {
    private static final String TAG = "cr.Geolocation";

    private CleanupRunable mCleanupRunable;
    private CleanupReference mCleanupReference;

    private static class CleanupRunable implements Runnable {
        private WeakReference<BisonContents> mBisonContents;
        private boolean mAllow;
        private boolean mRetain;
        private String mOrigin;

        public CleanupRunable(BisonContents bisonContents, String origin) {
            mBisonContents = new WeakReference<BisonContents>(bisonContents);
            mOrigin = origin;
        }

        @Override
        public void run() {
            assert ThreadUtils.runningOnUiThread();
            BisonContents awContents = mBisonContents.get();
            if (awContents == null) return;
            if (mRetain) {
                if (mAllow) {
                    awContents.getGeolocationPermissions().allow(mOrigin);
                } else {
                    awContents.getGeolocationPermissions().deny(mOrigin);
                }
            }
            awContents.invokeGeolocationCallback(mAllow, mOrigin);
        }

        public void setResponse(String origin, boolean allow, boolean retain) {
            mOrigin = origin;
            mAllow = allow;
            mRetain = retain;
        }
    }

    public BisonGeolocationCallback(String origin, BisonContents bisonContents) {
        mCleanupRunable = new CleanupRunable(bisonContents, origin);
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
