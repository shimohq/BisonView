

package im.shimo.bison.internal;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * The response information that is to be returned for a particular resource fetch.
 */
@JNINamespace("bison")
public class BisonWebResourceInterceptResponse {
    private BvWebResourceResponse mResponse;
    private boolean mRaisedException;

    public BisonWebResourceInterceptResponse(BvWebResourceResponse response, boolean raisedException) {
        mResponse = response;
        mRaisedException = raisedException;
    }

    @CalledByNative
    public BvWebResourceResponse getResponse() {
        return mResponse;
    }

    @CalledByNative
    public boolean getRaisedException() {
        return mRaisedException;
    }
}
