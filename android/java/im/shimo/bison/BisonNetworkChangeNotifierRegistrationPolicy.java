// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison;

import org.chromium.net.NetworkChangeNotifierAutoDetect;

/**
 * Registration policy to make sure we only listen to network changes when
 * there are live webview instances.
 */
public class BisonNetworkChangeNotifierRegistrationPolicy
        extends NetworkChangeNotifierAutoDetect.RegistrationPolicy
        implements BisonContentsLifecycleNotifier.Observer {

    @Override
    protected void init(NetworkChangeNotifierAutoDetect notifier) {
        super.init(notifier);
        BisonContentsLifecycleNotifier.addObserver(this);
    }

    @Override
    protected void destroy() {
        BisonContentsLifecycleNotifier.removeObserver(this);
    }

    // AwContentsLifecycleNotifier.Observer
    @Override
    public void onFirstWebViewCreated() {
        register();
    }

    @Override
    public void onLastWebViewDestroyed() {
        unregister();
    }
}
