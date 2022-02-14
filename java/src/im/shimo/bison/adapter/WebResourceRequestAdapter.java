package im.shimo.bison.adapter;

import android.net.Uri;
import androidx.annotation.RestrictTo;
import im.shimo.bison.WebResourceRequest;
import im.shimo.bison.internal.BvWebResourceRequest;

import java.util.Map;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public class WebResourceRequestAdapter implements WebResourceRequest {

    private final BvWebResourceRequest mRequest;

    public WebResourceRequestAdapter(BvWebResourceRequest request) {
        mRequest = request;
    }

    BvWebResourceRequest getBisonResourceRequest() {
        return mRequest;
    }

    @Override
    public Uri getUrl() {
        return Uri.parse(mRequest.url);
    }

    @Override
    public boolean isForMainFrame() {
        return mRequest.isMainFrame;
    }

    @Override
    public boolean hasGesture() {
        return mRequest.hasUserGesture;
    }

    @Override
    public String getMethod() {
        return mRequest.method;
    }

    @Override
    public Map<String, String> getRequestHeaders() {
        return mRequest.requestHeaders;
    }

    @Override
    public boolean isRedirect() {
        return mRequest.isRedirect;
    }

}
