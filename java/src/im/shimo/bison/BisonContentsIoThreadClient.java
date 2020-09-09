package im.shimo.bison;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

@JNINamespace("bison")
abstract class BisonContentsIoThreadClient {

    @CalledByNative
    public abstract int getCacheMode();

    @CalledByNative
    public abstract boolean shouldBlockContentUrls();

    @CalledByNative
    public abstract boolean shouldBlockFileUrls();

    @CalledByNative
    public abstract boolean shouldBlockNetworkLoads();

    @CalledByNative
    public abstract boolean shouldAcceptThirdPartyCookies();

    @CalledByNative
    public abstract boolean getSafeBrowsingEnabled();

    @CalledByNative
    public abstract BisonContentsBackgroundThreadClient getBackgroundThreadClient();
}
