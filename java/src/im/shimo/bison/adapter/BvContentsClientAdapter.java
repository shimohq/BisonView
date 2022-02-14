package im.shimo.bison.adapter;

import java.lang.reflect.Method;
import java.util.concurrent.Executor;

import org.chromium.base.TraceEvent;

import android.content.Context;
import android.util.Log;
import androidx.annotation.Nullable;
import im.shimo.bison.BisonView;
import im.shimo.bison.BisonViewClient;
import im.shimo.bison.BisonViewRenderProcessClient;
import im.shimo.bison.RenderProcessGoneDetail;
import im.shimo.bison.WebResourceResponse;
import im.shimo.bison.internal.BvContentsClient;
import im.shimo.bison.internal.BvRenderProcess;
import im.shimo.bison.internal.BvRenderProcessGoneDetail;
import im.shimo.bison.internal.BvWebResourceRequest;
import im.shimo.bison.internal.BvWebResourceResponse;

abstract class BvContentsClientAdapter extends BvContentsClient {

    protected static final String TAG = "BvContentsClientAdapter";

    static final BisonViewClient sNullBisonViewClient = new BisonViewClient();
    // Enables API callback tracing
    protected static final boolean TRACE = false;
    // The WebView instance that this adapter is serving.
    protected final BisonView mBisonView;
    // The Context to use. This is different from mWebView.getContext(), which
    // should not be used.
    protected final Context mContext;
    // A reference to the current WebViewClient associated with this WebView.
    protected BisonViewClient mBisonViewClient = sNullBisonViewClient;

    @Nullable
    private BisonViewRenderProcessClientAdapter mBisonViewRendererClientAdapter;

    private Method mGetStringMethod;

    /**
     * Adapter constructor.
     *
     * @param bisonView the {@link BisonView} instance that this adapter is serving.
     */
    public BvContentsClientAdapter(BisonView bisonView, Context context) {
        if (bisonView == null) {
            throw new IllegalArgumentException("bisonView can't be null.");
        }
        if (context == null) {
            throw new IllegalArgumentException("context can't be null.");
        }
        try {
            mGetStringMethod = Class.forName("android.net.http.ErrorStrings").getMethod("getString", int.class,
                    Context.class);
        } catch (Exception ignore) {
        }

        mBisonView = bisonView;
        mContext = context;
    }

    public void setBisonViewClient(BisonViewClient client) {
        mBisonViewClient = client != null ? client : sNullBisonViewClient;

    }

    public BisonViewClient getBisonViewClient() {
        return mBisonViewClient;
    }

    @Override
    public final boolean hasBisonViewClient() {
        return mBisonViewClient != sNullBisonViewClient;
    }

    /**
     * @see BvContentsClient#shouldOverrideUrlLoading(BvWebResourceRequest)
     */
    @Override
    public final boolean shouldOverrideUrlLoading(BvWebResourceRequest request) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.shouldOverrideUrlLoading");
            if (TRACE)
                Log.i(TAG, "shouldOverrideUrlLoading=" + request.url);
            boolean result = mBisonViewClient.shouldOverrideUrlLoading(mBisonView,
                    new WebResourceRequestAdapter(request));
            if (TRACE)
                Log.i(TAG, "shouldOverrideUrlLoading result=" + result);

            // Record UMA for shouldOverrideUrlLoading.
            // AwHistogramRecorder.recordCallbackInvocation(
            // AwHistogramRecorder.WebViewCallbackType.SHOULD_OVERRIDE_URL_LOADING);

            return result;
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.shouldOverrideUrlLoading");
        }
    }

    /**
     * @see ContentViewClient#onPageCommitVisible(String)
     */
    @Override
    public final void onPageCommitVisible(String url) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onPageCommitVisible");
            if (TRACE)
                Log.i(TAG, "onPageCommitVisible=" + url);
            mBisonViewClient.onPageCommitVisible(mBisonView, url);

            // Record UMA for onPageCommitVisible.
            // AwHistogramRecorder.recordCallbackInvocation(
            // AwHistogramRecorder.WebViewCallbackType.ON_PAGE_COMMIT_VISIBLE);

            // Otherwise, the API does not exist, so do nothing.
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onPageCommitVisible");
        }
    }

    /**
     * @see ContentViewClient#onReceivedError(int,String,String)
     */
    @Override
    public final void onReceivedError(int errorCode, String description, String failingUrl) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onReceivedError");
            if (description == null || description.isEmpty()) {
                description = getErrorString(mContext, errorCode);
            }
            if (TRACE)
                Log.i(TAG, "onReceivedError=" + failingUrl);
            mBisonViewClient.onReceivedError(mBisonView, errorCode, description, failingUrl);
        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onReceivedError");
        }
    }

    /**
     * @see ContentViewClient#onReceivedError(BvWebResourceRequest,AwWebResourceError)
     */
    @Override
    public void onReceivedError2(BvWebResourceRequest request, BvWebResourceError error) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onReceivedError");
            if (error.description == null || error.description.isEmpty()) {
                error.description = getErrorString(mContext, error.errorCode);
            }
            mBisonViewClient.onReceivedError(mBisonView, new WebResourceRequestAdapter(request),
                    new WebResourceErrorAdapter(error));

        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onReceivedError");
        }
    }

    @Override
    public void onReceivedHttpError(BvWebResourceRequest request, BvWebResourceResponse response) {
        try {
            TraceEvent.begin("WebViewContentsClientAdapter.onReceivedHttpError");
            if (TRACE)
                Log.i(TAG, "onReceivedHttpError=" + request.url);
            String reasonPhrase = response.getReasonPhrase();
            if (reasonPhrase == null || reasonPhrase.isEmpty()) {
                reasonPhrase = "UNKNOWN";
            }
            mBisonViewClient.onReceivedHttpError(mBisonView, new WebResourceRequestAdapter(request),
                    new WebResourceResponse(response.getMimeType(), response.getCharset(), response.getStatusCode(),
                            reasonPhrase, response.getResponseHeaders(), response.getData()));

        } finally {
            TraceEvent.end("WebViewContentsClientAdapter.onReceivedHttpError");
        }
    }

    public void setBisonViewRenderProcessClientAdapter(BisonViewRenderProcessClientAdapter adapter) {
        mBisonViewRendererClientAdapter = adapter;
    }

    BisonViewRenderProcessClientAdapter getBisonViewRendererClientAdapter() {
        return mBisonViewRendererClientAdapter;
    }

    @Override
    public void onRendererUnresponsive(final BvRenderProcess renderProcess) {
        if (mBisonViewRendererClientAdapter != null) {
            mBisonViewRendererClientAdapter.onRendererUnresponsive(mBisonView, renderProcess);
        }
    }

    @Override
    public void onRendererResponsive(final BvRenderProcess renderProcess) {
        if (mBisonViewRendererClientAdapter != null) {
            mBisonViewRendererClientAdapter.onRendererResponsive(mBisonView, renderProcess);
        }
    }

    @Override
    public boolean onRenderProcessGone(BvRenderProcessGoneDetail detail) {
        if (mBisonViewRendererClientAdapter == null)
            return true;
        return mBisonViewRendererClientAdapter.onRenderProcessGone(mBisonView, new RenderProcessGoneDetail() {

            @Override
            public boolean didCrash() {
                return detail.didCrash();
            }

            @Override
            public int rendererPriorityAtExit() {
                return detail.rendererPriority();
            }

        });
    }

    public String getErrorString(Context context, int errorCode) {
        try {
            return (String) mGetStringMethod.invoke(null, errorCode, context);
        } catch (Exception e) {
            if (TRACE)
                Log.w(TAG, e);
        }
        return "";
    }

}
