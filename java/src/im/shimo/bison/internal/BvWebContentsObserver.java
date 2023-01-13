package im.shimo.bison.internal;

//import im.shimo.bison.internal.BvContents.VisualStateCallback;
import org.chromium.base.task.PostTask;
import org.chromium.content_public.browser.GlobalRenderFrameHostId;
import org.chromium.content_public.browser.LifecycleState;
import org.chromium.content_public.browser.NavigationHandle;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.content_public.common.ContentUrlConstants;
import org.chromium.net.NetError;
import org.chromium.ui.base.PageTransition;
import org.chromium.url.GURL;

import java.lang.ref.WeakReference;

public class BvWebContentsObserver extends WebContentsObserver {

    private final WeakReference<BvContents> mBvContents;
    private final WeakReference<BvContentsClient> mBvContentsClient;

    private boolean mCommittedNavigation;

    private String mLastDidFinishLoadUrl;


    public BvWebContentsObserver(WebContents webContents, BvContents bvContents,
                                    BvContentsClient bvContentsClient) {
        super(webContents);
        mBvContents = new WeakReference<>(bvContents);
        mBvContentsClient = new WeakReference<>(bvContentsClient);
    }

    private BvContentsClient getClientIfNeedToFireCallback(String validatedUrl) {
        BvContentsClient client = mBvContentsClient.get();
        if (client != null) {
            String unreachableWebDataUrl = BvContentsStatics.getUnreachableWebDataUrl();
            if (unreachableWebDataUrl == null || !unreachableWebDataUrl.equals(validatedUrl)) {
                return client;
            }
        }
        return null;
    }

    @Override
    public void didFinishLoadInPrimaryMainFrame(GlobalRenderFrameHostId rfhId, GURL url,
            boolean isKnownValid, @LifecycleState int rfhLifecycleState) {
        if (rfhLifecycleState != LifecycleState.ACTIVE) return;
        String validatedUrl = isKnownValid ? url.getSpec() : url.getPossiblyInvalidSpec();
        if (getClientIfNeedToFireCallback(validatedUrl) != null) {
            mLastDidFinishLoadUrl = validatedUrl;
        }
    }

    @Override
    public void didStopLoading(GURL gurl, boolean isKnownValid) {
        String url = isKnownValid ? gurl.getSpec() : gurl.getPossiblyInvalidSpec();
        if (url.length() == 0) url = ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL;
        BvContentsClient client = getClientIfNeedToFireCallback(url);
        if (client != null && url.equals(mLastDidFinishLoadUrl)) {
           client.getCallbackHelper().postOnPageFinished(url);
            mLastDidFinishLoadUrl = null;
        }
    }

    @Override
    public void loadProgressChanged(float progress) {
        BvContentsClient client = mBvContentsClient.get();
        if (client == null) return;
        client.getCallbackHelper().postOnProgressChanged(Math.round(progress * 100));
    }

    @Override
    public void didFailLoad(boolean isInPrimaryMainFrame, @NetError int errorCode, GURL failingGurl,
            @LifecycleState int frameLifecycleState) {
        processFailedLoad(isInPrimaryMainFrame, errorCode, failingGurl);
    }

    private void processFailedLoad(
            boolean isPrimaryMainFrame, @NetError int errorCode, GURL failingGurl) {
        String failingUrl = failingGurl.getPossiblyInvalidSpec();
        BvContentsClient client = mBvContentsClient.get();
        if (client == null) return;
        String unreachableWebDataUrl = BvContentsStatics.getUnreachableWebDataUrl();
        boolean isErrorUrl =
                unreachableWebDataUrl != null && unreachableWebDataUrl.equals(failingUrl);
        if (isPrimaryMainFrame && !isErrorUrl) {
            if (errorCode == NetError.ERR_ABORTED) {
                // Need to call onPageFinished for backwards compatibility with the classic webview.
                // See also AwContentsClientBridge.onReceivedError.
                client.getCallbackHelper().postOnPageFinished(failingUrl);
            } else if (errorCode == NetError.ERR_HTTP_RESPONSE_CODE_FAILURE) {
                // This is a HTTP error that results in an error page. We need to call onPageStarted
                // and onPageFinished to have the same behavior with HTTP error navigations that
                // don't result in an error page. See also
                // AwContentsClientBridge.onReceivedHttpError.
                client.getCallbackHelper().postOnPageStarted(failingUrl);
                client.getCallbackHelper().postOnPageFinished(failingUrl);
            }
        }
    }

    @Override
    public void titleWasSet(String title) {
        BvContentsClient client = mBvContentsClient.get();
        if (client == null) return;
        client.updateTitle(title, true);
    }

    @Override
    public void didFinishNavigationInPrimaryMainFrame(NavigationHandle navigation) {
        String url = navigation.getUrl().getPossiblyInvalidSpec();
        if (navigation.errorCode() != NetError.OK && !navigation.isDownload()) {
          processFailedLoad(true, navigation.errorCode(), navigation.getUrl());
        }

        if (!navigation.hasCommitted()) return;

        mCommittedNavigation = true;

        BvContentsClient client = mBvContentsClient.get();
        if (client != null) {
            if (!navigation.isSameDocument() && !navigation.isErrorPage()
                && navigation.isRendererInitiated()) {
               client.getCallbackHelper().postOnPageStarted(url);
            }

            boolean isReload = (navigation.pageTransition() & PageTransition.CORE_MASK)
                    == PageTransition.RELOAD;
            client.getCallbackHelper().postDoUpdateVisitedHistory(url, isReload);
        }

        // Only invoke the onPageCommitVisible callback when navigating to a different document,
        // but not when navigating to a different fragment within the same document.
        if (!navigation.isSameDocument()) {
            PostTask.postTask(UiThreadTaskTraits.DEFAULT, () -> {
                BvContents bvContents = mBvContents.get();
                if (bvContents != null) {
                  //  bvContents.insertVisualStateCallbackIfNotDestroyed(
                  //          0, new VisualStateCallback() {
                  //              @Override
                  //              public void onComplete(long requestId) {
                  //                  BvContentsClient client1 = mBvContentsClient.get();
                  //                  if (client1 == null) return;
                  //                  client1.onPageCommitVisible(url);
                  //              }
                  //          });
                }
            });
        }

        if (client != null && navigation.isPrimaryMainFrameFragmentNavigation()) {
            // Note fragment navigations do not have a matching onPageStarted.
           client.getCallbackHelper().postOnPageFinished(url);
        }
    }

    public boolean didEverCommitNavigation() {
        return mCommittedNavigation;
    }
}
