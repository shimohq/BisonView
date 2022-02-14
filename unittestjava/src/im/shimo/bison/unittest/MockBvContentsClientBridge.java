// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.unittest;


import im.shimo.bison.internal.BvContentsClientBridge;
import im.shimo.bison.internal.ClientCertLookupTable;

import org.chromium.base.annotations.CalledByNative;

class MockBvContentsClientBridge extends BvContentsClientBridge {

    private int mId;
    private String[] mKeyTypes;

    public MockBvContentsClientBridge() {
        super(new ClientCertLookupTable());
    }

    @Override
    protected void selectClientCertificate(final int id, final String[] keyTypes,
            byte[][] encodedPrincipals, final String host, final int port) {
        mId = id;
        mKeyTypes = keyTypes;
    }

    @CalledByNative
    private static MockBvContentsClientBridge getBvContentsClientBridge() {
        return new MockBvContentsClientBridge();
    }

    @CalledByNative
    private String[] getKeyTypes() {
        return mKeyTypes;
    }

    @CalledByNative
    private int getRequestId() {
        return mId;
    }

    @CalledByNative
    private byte[][] createTestCertChain() {
        return new byte[][]{{1}};
    }
}
