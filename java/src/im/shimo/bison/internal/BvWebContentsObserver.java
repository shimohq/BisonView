package im.shimo.bison.internal;

import org.chromium.base.task.PostTask;
import org.chromium.content_public.browser.NavigationHandle;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.content_public.common.ContentUrlConstants;
import org.chromium.net.NetError;
import org.chromium.ui.base.PageTransition;

import java.lang.ref.WeakReference;

public class BvWebContentsObserver extends WebContentsObserver {

    private final WeakReference<BvContents> mBisonContents;
    private final WeakReference<BvContentsClient> mBisonContentsClient;

    private boolean mCommittedNavigation;

    private String mLastDidFinishLoadUrl;


    public BvWebContentsObserver(WebContents webContents, BvContents bvContents,
                                    BvContentsClient bvContentsClient) {
        super(webContents);
        mBisonContents = new WeakReference<>(bvContents);
        mBisonContentsClient = new WeakReference<>(bvContentsClient);
    }

    @Override
    public void didFinishLoad(long frameId, String validatedUrl, boolean isMainFrame) {
        if (isMainFrame ) {
            mLastDidFinishLoadUrl = validatedUrl;
        }
    }



    @Override
    public void didStopLoading(String validatedUrl) {
        if (validatedUrl.length() == 0) validatedUrl = ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL;
        BvContentsClient client = mBisonContentsClient.get();
        if (client != null && validatedUrl.equals(mLastDidFinishLoadUrl)) {
           client.getCallbackHelper().postOnPageFinished(validatedUrl);
            mLastDidFinishLoadUrl = null;
        }
    }

    @Override
    public void didFailLoad(
            boolean isMainFrame, @NetError int errorCode, String failingUrl) {
        BvContentsClient client = mBisonContentsClient.get();
        if (client == null) return;
        if (isMainFrame && errorCode == NetError.ERR_ABORTED) {
            // Need to call onPageFinished for backwards compatibility with the classic webview.
            // See also AwContents.IoThreadClientImpl.onReceivedError.
           client.getCallbackHelper().postOnPageFinished(failingUrl);

        }
    }

    @Override
    public void titleWasSet(String title) {
        BvContentsClient client = mBisonContentsClient.get();
        if (client == null) return;
        client.updateTitle(title, true);
    }

    @Override
    public void didFinishNavigation(NavigationHandle navigation) {
        String url = navigation.getUrl();
        if (navigation.errorCode() != 0 && !navigation.isDownload()) {
            didFailLoad(navigation.isInMainFrame(), navigation.errorCode(), url);
        }

        if (!navigation.hasCommitted()) return;

        mCommittedNavigation = true;

        if (!navigation.isInMainFrame()) return;

        BvContentsClient client = mBisonContentsClient.get();
        if (client != null) {
            if (!navigation.isSameDocument() && !navigation.isErrorPage()
                && navigation.isRendererInitiated()) {
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
                BvContents bvContents = mBisonContents.get();
                if (bvContents != null) {
                //    bvContents.insertVisualStateCallbackIfNotDestroyed(
                //            0, new VisualStateCallback() {
                //                @Override
                //                public void onComplete(long requestId) {
                //                    BvContentsClient client1 = client.get();
                //                    if (client1 == null) return;
                //                    client1.onPageCommitVisible(url);
                //                }
                //            });
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
