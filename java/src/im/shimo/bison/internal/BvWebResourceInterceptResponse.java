

package im.shimo.bison.internal;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.components.embedder_support.util.WebResourceResponseInfo;

/**
 * The response information that is to be returned for a particular resource fetch.
 */
@JNINamespace("bison")
public class BvWebResourceInterceptResponse {
  private WebResourceResponseInfo mResponse;
    private boolean mRaisedException;

    public BvWebResourceInterceptResponse(WebResourceResponseInfo response, boolean raisedException) {
        mResponse = response;
        mRaisedException = raisedException;
    }

    @CalledByNative
    public WebResourceResponseInfo getResponse() {
        return mResponse;
    }

    @CalledByNative
    public boolean getRaisedException() {
        return mRaisedException;
    }
}
