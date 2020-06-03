// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.core;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * The response information that is to be returned for a particular resource fetch.
 */
@JNINamespace("bison")
public class BisonWebResourceInterceptResponse {
    private BisonWebResourceResponse mResponse;
    private boolean mRaisedException;

    public BisonWebResourceInterceptResponse(BisonWebResourceResponse response, boolean raisedException) {
        mResponse = response;
        mRaisedException = raisedException;
    }

    @CalledByNative
    public BisonWebResourceResponse getResponse() {
        return mResponse;
    }

    @CalledByNative
    public boolean getRaisedException() {
        return mRaisedException;
    }
}
