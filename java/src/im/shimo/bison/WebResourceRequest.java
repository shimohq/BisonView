package im.shimo.bison;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.HashMap;

public class WebResourceRequest {

    // Url of the request.
    String url;

    boolean isMainFrame;

    boolean hasUserGesture;

    boolean isRedirect;
    // (GET/POST/OPTIONS)
    String method;

    HashMap<String, String> requestHeaders;

    public WebResourceRequest() {}

    public WebResourceRequest(String url, boolean isMainFrame, boolean hasUserGesture,
                              String method, @Nullable HashMap<String, String> requestHeaders) {
        this.url = url;
        this.isMainFrame = isMainFrame;
        this.hasUserGesture = hasUserGesture;
        this.method = method;
        this.requestHeaders = requestHeaders;
    }


    public WebResourceRequest(String url, boolean isMainFrame, boolean hasUserGesture,
                              String method, @NonNull String[] requestHeaderNames,
                              @NonNull String[] requestHeaderValues) {
        this(url, isMainFrame, hasUserGesture, method,
                new HashMap<String, String>(requestHeaderValues.length));
        for (int i = 0; i < requestHeaderNames.length; ++i) {
            this.requestHeaders.put(requestHeaderNames[i], requestHeaderValues[i]);
        }
    }


    public String getUrl() {
        return url;
    }

    public boolean isMainFrame() {
        return isMainFrame;
    }

    public boolean isHasUserGesture() {
        return hasUserGesture;
    }

    public boolean isRedirect() {
        return isRedirect;
    }

    public String getMethod() {
        return method;
    }

    public HashMap<String, String> getRequestHeaders() {
        return requestHeaders;
    }
}
