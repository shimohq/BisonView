// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.core;

import org.chromium.base.task.PostTask;
import org.chromium.content_public.browser.UiThreadTaskTraits;

class JsResultHandler implements JsResultReceiver, JsPromptResultReceiver {
    private BisonContentsClientBridge mBridge;
    private final int mId;

    JsResultHandler(BisonContentsClientBridge bridge, int id) {
        mBridge = bridge;
        mId = id;
    }

    @Override
    public void confirm() {
        confirm(null);
    }

    @Override
    public void confirm(final String promptResult) {
        PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> {
            if (mBridge != null) mBridge.confirmJsResult(mId, promptResult);
            mBridge = null;
        });
    }

    @Override
    public void cancel() {
        PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> {
            if (mBridge != null) mBridge.cancelJsResult(mId);
            mBridge = null;
        });
    }
}
