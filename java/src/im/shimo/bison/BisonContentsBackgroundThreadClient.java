package im.shimo.bison;


import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

@JNINamespace("bison")
abstract class BisonContentsBackgroundThreadClient {
    private static final String TAG = "BisonContentsBackground";

    public abstract BisonWebResourceResponse shouldInterceptRequest(
            BisonContentsClient.BisonWebResourceRequest request);


    @CalledByNative
    private BisonWebResourceInterceptResponse shouldInterceptRequestFromNative(String url,
                                                                               boolean isMainFrame,
                                                                               boolean hasUserGesture,
                                                                               String method,
                                                                               String[] requestHeaderNames,
                                                                               String[] requestHeaderValues) {
        try {
            return new BisonWebResourceInterceptResponse(
                    shouldInterceptRequest(new BisonContentsClient.BisonWebResourceRequest(url,
                            isMainFrame, hasUserGesture, method, requestHeaderNames,
                            requestHeaderValues)), false);
        } catch (Exception e) {
            Log.e(TAG,
                    "Client raised exception in shouldInterceptRequest. Re-throwing on UI thread.");
            ThreadUtils.getUiThreadHandler().post(() -> {
                Log.e(TAG, "The following exception was raised by shouldInterceptRequest:");
                throw e;
            });

            return new BisonWebResourceInterceptResponse(null, true);
        }
    }


}
