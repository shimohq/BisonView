package im.shimo.bison;


import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

@JNINamespace("bison")
abstract class BisonContentsBackgroundThreadClient {
    private static final String TAG = "BisonContentsBackground";

    public abstract BisonWebResourceResponse shouldInterceptRequest(
        BisonWebResourceRequest request);


    @CalledByNative
    private BisonWebResourceInterceptResponse shouldInterceptRequestFromNative(String url,
                                                                               boolean isMainFrame,
                                                                               boolean hasUserGesture,
                                                                               String method,
                                                                               String[] requestHeaderNames,
                                                                               String[] requestHeaderValues) {
        try {
            return new BisonWebResourceInterceptResponse(
                    shouldInterceptRequest(new BisonWebResourceRequest(url,
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


    @CalledByNative
    private BisonWebResourceOverriteRequest shouldOverriteRequestFromNative(String url,
                                                                    boolean isMainFrame,
                                                                    boolean hasUserGesture,
                                                                    String method,
                                                                    String[] requestHeaderNames,
                                                                    String[] requestHeaderValues){
        try {
            BisonWebResourceRequest request = new BisonWebResourceRequest(url,
                    isMainFrame, hasUserGesture, method, requestHeaderNames,
                    requestHeaderValues);
            shouldOverriteRequest(request);
            return new BisonWebResourceOverriteRequest(request,false);
        } catch (Exception e) {
            Log.e(TAG,
                    "Client raised exception in shouldOverriteRequest. Re-throwing on UI thread.");
            ThreadUtils.getUiThreadHandler().post(() -> {
                Log.e(TAG, "The following exception was raised by shouldOverriteRequest:");
                throw e;
            });

            return new BisonWebResourceOverriteRequest(null,true);
        }
    }

    public abstract void shouldOverriteRequest(BisonWebResourceRequest request);


}
