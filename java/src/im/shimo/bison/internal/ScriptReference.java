// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.internal;

import org.chromium.base.ThreadUtils;

import java.lang.ref.WeakReference;

/**
 * Used for Js Java interaction, to delete the document start JavaScript snippet.
 */
public class ScriptReference {
    private WeakReference<BvContents> mBvContentsRef;
    private int mScriptId;

    public ScriptReference(BvContents bvContents, int scriptId) {
        assert scriptId >= 0;
        mBvContentsRef = new WeakReference<>(bvContents);
        mScriptId = scriptId;
    }

    // Must be called on UI thread.
    public void remove() {
        ThreadUtils.checkUiThread();

        BvContents bvContents = mBvContentsRef.get();
        if (bvContents == null) return;
        bvContents.removeDocumentStartJavaScript(mScriptId);
    }
}
