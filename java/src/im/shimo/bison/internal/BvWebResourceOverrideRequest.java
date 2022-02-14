package im.shimo.bison.internal;

import java.util.Map;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;


@JNINamespace("bison")
public class BvWebResourceOverrideRequest {

    private BvWebResourceRequest mRequest;
    private boolean mRaisedException;

    private String[] mRequestHeaderNames;
    private String[] mRequestHeaderValues;

    public BvWebResourceOverrideRequest(BvWebResourceRequest request,
            boolean raisedException) {
        mRequest = request;
        mRaisedException = raisedException;
    }

    private void fillInRequestHeaderNamesAndValuesIfNeeded() {
        if (mRequest == null || mRequest.requestHeaders == null)
            return;
        mRequestHeaderNames = new String[mRequest.requestHeaders.size()];
        mRequestHeaderValues = new String[mRequest.requestHeaders.size()];
        int i = 0;
        for (Map.Entry<String, String> entry : mRequest.requestHeaders.entrySet()) {
            mRequestHeaderNames[i] = entry.getKey();
            mRequestHeaderValues[i] = entry.getValue();
            i++;
        }
    }

    @CalledByNative
    public boolean getRaisedException() {
        return mRaisedException;
    }

    @CalledByNative
    public BvWebResourceRequest getRequest() {
        return mRequest;
    }

    @CalledByNative
    public String getRequestUrl() {
        return mRequest.url;
    }

    @CalledByNative
    public String getRequestMethod() {
        return mRequest.method;
    }

    @CalledByNative
    private String[] getRequestHeaderNames() {
        fillInRequestHeaderNamesAndValuesIfNeeded();
        return mRequestHeaderNames;
    }

    @CalledByNative
    private String[] getRequestHeaderValues() {
        fillInRequestHeaderNamesAndValuesIfNeeded();
        return mRequestHeaderValues;
    }

}
