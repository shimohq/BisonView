// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison;

import im.shimo.bison.BisonContentsClient.BisonWebResourceRequest;

/**
 * Abstract base class that implementors of service worker related callbacks
 * derive from.
 */
public abstract class BisonServiceWorkerClient {

    public abstract BisonWebResourceResponse shouldInterceptRequest(BisonWebResourceRequest request);

    // TODO: add support for onReceivedError and onReceivedHttpError callbacks.
}