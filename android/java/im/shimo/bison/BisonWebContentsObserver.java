// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison;

import im.shimo.bison.BisonContents.VisualStateCallback;
import org.chromium.base.task.PostTask;
import org.chromium.content_public.browser.NavigationHandle;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.content_public.common.ContentUrlConstants;
import org.chromium.net.NetError;
import org.chromium.ui.base.PageTransition;

import java.lang.ref.WeakReference;

/**
 * Routes notifications from WebContents to AwContentsClient and other listeners.
 */
public class BisonWebContentsObserver extends WebContentsObserver {
    // TODO(tobiasjs) similarly to WebContentsObserver.mWebContents, mBisonContents
    // needs to be a WeakReference, which suggests that there exists a strong
    // reference to an AwWebContentsObserver instance. This is not intentional,
    // and should be found and cleaned up.
    private final WeakReference<BisonContents> mBisonContents;
    private final WeakReference<BisonContentsClient> mAwContentsClient;

    // Whether this webcontents has ever committed any navigation.
    private boolean mCommittedNavigation;

    // Temporarily stores the URL passed the last time to didFinishLoad callback.
    private String mLastDidFinishLoadUrl;

    public BisonWebContentsObserver(
            WebContents webContents, BisonContents bisonContents, BisonContentsClient awContentsClient) {
        super(webContents);
        mBisonContents = new WeakReference<>(bisonContents);
        mAwContentsClient = new WeakReference<>(awContentsClient);
    }

    private BisonContentsClient getClientIfNeedToFireCallback(String validatedUrl) {
        BisonContentsClient client = mAwContentsClient.get();
        if (client != null) {
            String unreachableWebDataUrl = BisonContentsStatics.getUnreachableWebDataUrl();
            if (unreachableWebDataUrl == null || !unreachableWebDataUrl.equals(validatedUrl)) {
                return client;
            }
        }
        return null;
    }

    @Override
    public void didFinishLoad(long frameId, String validatedUrl, boolean isMainFrame) {
        if (isMainFrame && getClientIfNeedToFireCallback(validatedUrl) != null) {
            mLastDidFinishLoadUrl = validatedUrl;
        }
    }

    @Override
    public void didStopLoading(String validatedUrl) {
        if (validatedUrl.length() == 0) validatedUrl = ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL;
        BisonContentsClient client = getClientIfNeedToFireCallback(validatedUrl);
        if (client != null && validatedUrl.equals(mLastDidFinishLoadUrl)) {
            client.getCallbackHelper().postOnPageFinished(validatedUrl);
            mLastDidFinishLoadUrl = null;
        }
    }

    @Override
    public void didFailLoad(
            boolean isMainFrame, @NetError int errorCode, String description, String failingUrl) {
        BisonContentsClient client = mAwContentsClient.get();
        if (client == null) return;
        String unreachableWebDataUrl = BisonContentsStatics.getUnreachableWebDataUrl();
        boolean isErrorUrl =
                unreachableWebDataUrl != null && unreachableWebDataUrl.equals(failingUrl);
        if (isMainFrame && !isErrorUrl && errorCode == NetError.ERR_ABORTED) {
            // Need to call onPageFinished for backwards compatibility with the classic webview.
            // See also AwContents.IoThreadClientImpl.onReceivedError.
            client.getCallbackHelper().postOnPageFinished(failingUrl);
        }
    }

    @Override
    public void titleWasSet(String title) {
        BisonContentsClient client = mAwContentsClient.get();
        if (client == null) return;
        client.updateTitle(title, true);
    }

    @Override
    public void didFinishNavigation(NavigationHandle navigation) {
        String url = navigation.getUrl();
        if (navigation.errorCode() != 0 && !navigation.isDownload()) {
            didFailLoad(navigation.isInMainFrame(), navigation.errorCode(),
                    navigation.errorDescription(), url);
        }

        if (!navigation.hasCommitted()) return;

        mCommittedNavigation = true;

        if (!navigation.isInMainFrame()) return;

        BisonContentsClient client = mAwContentsClient.get();
        if (client != null) {
            // OnPageStarted is not called for in-page navigations, which include fragment
            // navigations and navigation from history.push/replaceState.
            // Error page is handled by AwContentsClientBridge.onReceivedError.
            if (!navigation.isSameDocument() && !navigation.isErrorPage()
                    && BisonFeatureList.pageStartedOnCommitEnabled(navigation.isRendererInitiated())) {
                client.getCallbackHelper().postOnPageStarted(url);
            }

            boolean isReload = navigation.pageTransition() != null
                    && ((navigation.pageTransition() & PageTransition.CORE_MASK)
                            == PageTransition.RELOAD);
            client.getCallbackHelper().postDoUpdateVisitedHistory(url, isReload);
        }

        // Only invoke the onPageCommitVisible callback when navigating to a different document,
        // but not when navigating to a different fragment within the same document.
        if (!navigation.isSameDocument()) {
            PostTask.postTask(UiThreadTaskTraits.DEFAULT, () -> {
                BisonContents awContents = mBisonContents.get();
                if (awContents != null) {
                    awContents.insertVisualStateCallbackIfNotDestroyed(
                            0, new VisualStateCallback() {
                                @Override
                                public void onComplete(long requestId) {
                                    BisonContentsClient client1 = mAwContentsClient.get();
                                    if (client1 == null) return;
                                    client1.onPageCommitVisible(url);
                                }
                            });
                }
            });
        }

        if (client != null && navigation.isFragmentNavigation()) {
            // Note fragment navigations do not have a matching onPageStarted.
            client.getCallbackHelper().postOnPageFinished(url);
        }
    }

    public boolean didEverCommitNavigation() {
        return mCommittedNavigation;
    }
}
