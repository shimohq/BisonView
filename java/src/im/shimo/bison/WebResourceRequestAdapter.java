package im.shimo.bison;

import android.net.Uri;

import java.util.Map;

class WebResourceRequestAdapter implements WebResourceRequest {

    private final BisonWebResourceRequest mRequest;

    public WebResourceRequestAdapter(BisonWebResourceRequest request) {
        mRequest = request;
    }

    BisonWebResourceRequest getBisonResourceRequest() {
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
