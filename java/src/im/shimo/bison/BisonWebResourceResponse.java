package im.shimo.bison;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.io.InputStream;
import java.util.Map;

/**
 * The response information that is to be returned for a particular resource fetch.
 */
@JNINamespace("bison")
public class BisonWebResourceResponse {
    private String mMimeType;
    private String mCharset;
    private InputStream mData;
    private int mStatusCode;
    private String mReasonPhrase;
    private Map<String, String> mResponseHeaders;
    private String[] mResponseHeaderNames;
    private String[] mResponseHeaderValues;

    public BisonWebResourceResponse(String mimeType, String encoding, InputStream data) {
        mMimeType = mimeType;
        mCharset = encoding;
        mData = data;
    }

    public BisonWebResourceResponse(String mimeType, String encoding, InputStream data,
                                    int statusCode, String reasonPhrase, Map<String, String> responseHeaders) {
        this(mimeType, encoding, data);

        mStatusCode = statusCode;
        mReasonPhrase = reasonPhrase;
        mResponseHeaders = responseHeaders;
    }

    private void fillInResponseHeaderNamesAndValuesIfNeeded() {
        if (mResponseHeaders == null || mResponseHeaderNames != null) return;
        mResponseHeaderNames = new String[mResponseHeaders.size()];
        mResponseHeaderValues = new String[mResponseHeaders.size()];
        int i = 0;
        for (Map.Entry<String, String> entry : mResponseHeaders.entrySet()) {
            mResponseHeaderNames[i] = entry.getKey();
            mResponseHeaderValues[i] = entry.getValue();
            i++;
        }
    }

    @CalledByNative
    public String getMimeType() {
        return mMimeType;
    }

    @CalledByNative
    public String getCharset() {
        return mCharset;
    }

    @CalledByNative
    public InputStream getData() {
        return mData;
    }

    @CalledByNative
    public int getStatusCode() {
        return mStatusCode;
    }

    @CalledByNative
    public String getReasonPhrase() {
        return mReasonPhrase;
    }

    public Map<String, String> getResponseHeaders() {
        return mResponseHeaders;
    }

    @CalledByNative
    private String[] getResponseHeaderNames() {
        fillInResponseHeaderNamesAndValuesIfNeeded();
        return mResponseHeaderNames;
    }

    @CalledByNative
    private String[] getResponseHeaderValues() {
        fillInResponseHeaderNamesAndValuesIfNeeded();
        return mResponseHeaderValues;
    }
}
