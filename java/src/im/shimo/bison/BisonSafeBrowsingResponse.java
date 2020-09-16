
package im.shimo.bison;

/**
 * Container to hold the application's response to WebViewClient#onSafeBrowsingHit().
 */
class BisonSafeBrowsingResponse {
    private int mAction;
    private boolean mReporting;

    public BisonSafeBrowsingResponse(int action, boolean reporting) {
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
