package im.shimo.bison.internal;


import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
@JNINamespace("bison")
abstract class BvContentsBackgroundThreadClient {
    private static final String TAG = "BisonContentsBackground";

    public abstract BvWebResourceResponse shouldInterceptRequest(
            BvWebResourceRequest request);


    @CalledByNative
    private BisonWebResourceInterceptResponse shouldInterceptRequestFromNative(String url,
            boolean isMainFrame, boolean hasUserGesture, String method, String[] requestHeaderNames,
            String[] requestHeaderValues) {
        try {
            return new BisonWebResourceInterceptResponse(
                    shouldInterceptRequest(new BvWebResourceRequest(url, isMainFrame,
                            hasUserGesture, method, requestHeaderNames, requestHeaderValues)),
                    false);
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
    private BvWebResourceOverrideRequest overrideRequestFromNative(String url,
            boolean isMainFrame, boolean hasUserGesture, String method, String[] requestHeaderNames,
            String[] requestHeaderValues) {
        try {
            BvWebResourceRequest request = new BvWebResourceRequest(url, isMainFrame,
                    hasUserGesture, method, requestHeaderNames, requestHeaderValues);
            overrideRequest(request);
            return new BvWebResourceOverrideRequest(request, false);
        } catch (Exception e) {
            Log.e(TAG, "Client raised exception in overrideRequest. Re-throwing on UI thread.");
            ThreadUtils.getUiThreadHandler().post(() -> {
                Log.e(TAG, "The following exception was raised by overrideRequest:");
                throw e;
            });

            return new BvWebResourceOverrideRequest(null, true);
        }
    }

    public abstract void overrideRequest(BvWebResourceRequest request);


}
