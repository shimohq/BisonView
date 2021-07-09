package im.shimo.bison;

import java.io.InputStream;
import java.util.Map;

public class WebResourceResponse {


    private final String mMimeType;
    private final String mCharset;
    private final int mStatusCode;
    private final String mReasonPhrase;
    private final Map<String, String> mResponseHeaders;
    private final InputStream mData;

    public WebResourceResponse(String mimeType, String charset, int statusCode, String reasonPhrase,
                               Map<String, String> responseHeaders, InputStream data) {

        mMimeType = mimeType;
        mCharset = charset;
        mStatusCode = statusCode;
        mReasonPhrase = reasonPhrase;
        mResponseHeaders = responseHeaders;
        mData = data;
    }


    public String getMimeType() {
        return mMimeType;
    }

    public String getCharset() {
        return mCharset;
    }

    public int getStatusCode() {
        return mStatusCode;
    }

    public String getReasonPhrase() {
        return mReasonPhrase;
    }

    public Map<String, String> getResponseHeaders() {
        return mResponseHeaders;
    }

    public InputStream getData() {
        return mData;
    }
}
