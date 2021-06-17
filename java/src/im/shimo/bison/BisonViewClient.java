package im.shimo.bison;

import android.graphics.Bitmap;
import android.net.http.SslError;
import android.webkit.SslErrorHandler;
import android.webkit.WebResourceError;

public class BisonViewClient {

    public boolean shouldOverrideUrlLoading(BisonView view, WebResourceRequest request) {
        return false;
    }


    public void onPageStarted(BisonView view, String url, Bitmap favicon) {

    }


    public void onPageFinished(BisonView view, String url) {

    }

    public void onLoadResource(BisonView view, String url) {
    }

    public void onPageCommitVisible(BisonView view, String url) {
    }

    public WebResourceResponse shouldInterceptRequest(BisonView view, WebResourceRequest request) {
        return null;
    }

    public void onReceivedError(BisonView view, WebResourceRequest request, WebResourceError error) {
        if (request.isMainFrame()) {

        }
    }

    public void onReceivedHttpError(
            BisonView view, WebResourceRequest request, WebResourceResponse errorResponse) {
    }

    /**
     * Notify the host application to update its visited links database.
     * @param view The BisonView that is initiating the callback.
     * @param url The url being visited.
     * @param isReload  {@code true} if this url is being reloaded.
     */
    public void doUpdateVisitedHistory(BisonView view, String url, boolean isReload) {
    }

    public void onReceivedSslError(BisonView view, SslErrorHandler handler,
                                   SslError error) {
        //handler.cancel();
    }

    public void onReceivedClientCertRequest(BisonView view, ClientCertRequest request) {
        //request.cancel();
    }




}
