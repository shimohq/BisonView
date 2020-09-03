package im.shimo.bison;

import org.chromium.base.task.PostTask;
import org.chromium.content_public.browser.NavigationHandle;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.content_public.common.ContentUrlConstants;
import org.chromium.net.NetError;
import org.chromium.ui.base.PageTransition;

import java.lang.ref.WeakReference;

public class BisonWebContentsObserver extends WebContentsObserver {

    private final WeakReference<BisonContents> mBisonContents;
    private final WeakReference<BisonContentsClient> mBisonContentsClient;

    private boolean mCommittedNavigation;

    private String mLastDidFinishLoadUrl;


    public BisonWebContentsObserver(WebContents webContents, BisonContents bisonContents,
                                    BisonContentsClient bisonContentsClient) {
        super(webContents);
        mBisonContents = new WeakReference<>(bisonContents);
        mBisonContentsClient = new WeakReference<>(bisonContentsClient);
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
        BisonContentsClient client = mBisonContentsClient.get();
        if (client != null && validatedUrl.equals(mLastDidFinishLoadUrl)) {
//            client.getCallbackHelper().postOnPageFinished(validatedUrl);
            client.onPageFinished(validatedUrl);
            mLastDidFinishLoadUrl = null;
        }
    }

    @Override
    public void didFailLoad(
            boolean isMainFrame, @NetError int errorCode, String description, String failingUrl) {
        BisonContentsClient client = mBisonContentsClient.get();
        if (client == null) return;
        if (isMainFrame && errorCode == NetError.ERR_ABORTED) {
            // Need to call onPageFinished for backwards compatibility with the classic webview.
            // See also AwContents.IoThreadClientImpl.onReceivedError.
//            client.getCallbackHelper().postOnPageFinished(failingUrl);
            client.onPageFinished(failingUrl);
        }
    }

    @Override
    public void titleWasSet(String title) {
        BisonContentsClient client = mBisonContentsClient.get();
        if (client == null) return;
        //client.updateTitle(title, true);
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

        BisonContentsClient client = mBisonContentsClient.get();
        if (client != null) {

            if (!navigation.isSameDocument() && !navigation.isErrorPage()) {
//                client.getCallbackHelper().postOnPageStarted(url);
                client.onPageStarted(url);
            }

            boolean isReload = navigation.pageTransition() != null
                    && ((navigation.pageTransition() & PageTransition.CORE_MASK)
                    == PageTransition.RELOAD);
            //client.getCallbackHelper().postDoUpdateVisitedHistory(url, isReload);
        }

        // Only invoke the onPageCommitVisible callback when navigating to a different document,
        // but not when navigating to a different fragment within the same document.
        if (!navigation.isSameDocument()) {
            PostTask.postTask(UiThreadTaskTraits.DEFAULT, () -> {
                BisonContents bisonContents = mBisonContents.get();
                if (bisonContents != null) {
//                    awContents.insertVisualStateCallbackIfNotDestroyed(
//                            0, new VisualStateCallback() {
//                                @Override
//                                public void onComplete(long requestId) {
//                                    AwContentsClient client1 = mAwContentsClient.get();
//                                    if (client1 == null) return;
//                                    client1.onPageCommitVisible(url);
//                                }
//                            });
                }
            });
        }

        if (client != null && navigation.isFragmentNavigation()) {
            // Note fragment navigations do not have a matching onPageStarted.
//            client.getCallbackHelper().postOnPageFinished(url);
            client.onPageFinished(url);
        }
    }


}
