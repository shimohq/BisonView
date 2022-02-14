
package im.shimo.bison.internal;

import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
class BvSafeBrowsingResponse {
    private int mAction;
    private boolean mReporting;

    public BvSafeBrowsingResponse(int action, boolean reporting) {
        mAction = action;
        mReporting = reporting;
    }

    public int action() {
        return mAction;
    }
    public boolean reporting() {
        return mReporting;
    }
}
