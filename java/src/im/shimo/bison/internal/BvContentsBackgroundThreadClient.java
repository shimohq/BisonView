package im.shimo.bison.internal;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.components.embedder_support.util.WebResourceResponseInfo;



@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
@JNINamespace("bison")
abstract class BvContentsBackgroundThreadClient {
    private static final String TAG = "BisonContentsBackground";

    public abstract WebResourceResponseInfo shouldInterceptRequest(
            BvWebResourceRequest request);

    // Protected methods ---------------------------------------------------------------------------

    @NonNull
    @CalledByNative
    private BvWebResourceInterceptResponse shouldInterceptRequestFromNative(String url,
            boolean isMainFrame, boolean hasUserGesture, String method, String[] requestHeaderNames,
            String[] requestHeaderValues) {
        try {
            return new BvWebResourceInterceptResponse(
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

            return new BvWebResourceInterceptResponse(null, true);
        }
    }

    // jiang
    //@CalledByNative
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
