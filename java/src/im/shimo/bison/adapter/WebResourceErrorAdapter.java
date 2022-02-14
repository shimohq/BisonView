package im.shimo.bison.adapter;

import im.shimo.bison.WebResourceError;
import im.shimo.bison.internal.BvContentsClient;

public class WebResourceErrorAdapter extends WebResourceError {

    private final BvContentsClient.BvWebResourceError mError;

    public WebResourceErrorAdapter(BvContentsClient.BvWebResourceError error) {
        mError = error;
    }

    /* package */ BvContentsClient.BvWebResourceError getBvWebResourceError() {
        return mError;
    }

    @Override
    public int getErrorCode() {
        return mError.errorCode;
    }

    @Override
    public CharSequence getDescription() {
        return mError.description;
    }
}
