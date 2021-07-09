package im.shimo.bison;

import android.os.Handler;

public class SslErrorHandler extends Handler {

    public SslErrorHandler() {}

    /**
     * Proceed with the SSL certificate.
     * <p>
     * It is not recommended to proceed past SSL errors and this method should
     * generally not be used; see {@link BisonViewClient#onReceivedSslError} for
     * more information.
     */
    public void proceed() {}

    /**
     * Cancel this request and all pending requests for the WebView that had
     * the error.
     */
    public void cancel() {}
}
