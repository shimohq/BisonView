// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.safe_browsing;

/**
 * Container to hold the application's response to WebViewClient#onSafeBrowsingHit().
 */
public class BisonSafeBrowsingResponse {
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
