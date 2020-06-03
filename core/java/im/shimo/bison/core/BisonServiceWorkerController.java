// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.core;

import android.content.Context;

import im.shimo.bison.core.safe_browsing.BisonSafeBrowsingConfigHelper;

/**
 * Manages clients and settings for Service Workers.
 */
public class BisonServiceWorkerController {
    private BisonServiceWorkerClient mServiceWorkerClient;
    private BisonContentsIoThreadClient mServiceWorkerIoThreadClient;
    private BisonContentsBackgroundThreadClient mServiceWorkerBackgroundThreadClient;
    private BisonServiceWorkerSettings mServiceWorkerSettings;
    private BisonBrowserContext mBrowserContext;

    public BisonServiceWorkerController(Context applicationContext, BisonBrowserContext browserContext) {
        mServiceWorkerSettings = new BisonServiceWorkerSettings(applicationContext);
        mBrowserContext = browserContext;
    }

    /**
     * Returns the current settings for Service Worker.
     */
    public BisonServiceWorkerSettings getBisonServiceWorkerSettings() {
        return mServiceWorkerSettings;
    }

    /**
     * Set custom client to receive callbacks from Service Workers. Can be null.
     */
    public void setServiceWorkerClient(BisonServiceWorkerClient client) {
        mServiceWorkerClient = client;
        if (client != null) {
            mServiceWorkerBackgroundThreadClient = new ServiceWorkerBackgroundThreadClientImpl();
            mServiceWorkerIoThreadClient = new ServiceWorkerIoThreadClientImpl();
            BisonContentsStatics.setServiceWorkerIoThreadClient(
                    mServiceWorkerIoThreadClient, mBrowserContext);
        } else {
            mServiceWorkerBackgroundThreadClient = null;
            mServiceWorkerIoThreadClient = null;
            BisonContentsStatics.setServiceWorkerIoThreadClient(null, mBrowserContext);
        }
    }

    // Helper classes implementations

    private class ServiceWorkerIoThreadClientImpl extends BisonContentsIoThreadClient {
        // All methods are called on the IO thread.

        @Override
        public int getCacheMode() {
            return mServiceWorkerSettings.getCacheMode();
        }

        @Override
        public BisonContentsBackgroundThreadClient getBackgroundThreadClient() {
            return mServiceWorkerBackgroundThreadClient;
        }

        @Override
        public boolean shouldBlockContentUrls() {
            return !mServiceWorkerSettings.getAllowContentAccess();
        }

        @Override
        public boolean shouldBlockFileUrls() {
            return !mServiceWorkerSettings.getAllowFileAccess();
        }

        @Override
        public boolean shouldBlockNetworkLoads() {
            return mServiceWorkerSettings.getBlockNetworkLoads();
        }

        @Override
        public boolean shouldAcceptThirdPartyCookies() {
            // We currently don't allow third party cookies in service workers,
            // see e.g. BisonCookieAccessPolicy::GetShouldAcceptThirdPartyCookies.
            return false;
        }

        @Override
        public boolean getSafeBrowsingEnabled() {
            return BisonSafeBrowsingConfigHelper.getSafeBrowsingEnabledByManifest();
        }
    }

    private class ServiceWorkerBackgroundThreadClientImpl
            extends BisonContentsBackgroundThreadClient {
        // All methods are called on the background thread.
        @Override
        public BisonWebResourceResponse shouldInterceptRequest(
                BisonContentsClient.BisonWebResourceRequest request) {
            // TODO: Consider analogy with BisonContentsClient, i.e.
            //  - do we need an onloadresource callback?
            //  - do we need to post an error if the response data == null?
            return mServiceWorkerClient.shouldInterceptRequest(request);
        }
    }
}
