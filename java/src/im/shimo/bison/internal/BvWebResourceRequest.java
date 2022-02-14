package im.shimo.bison.internal;

import java.util.HashMap;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class BvWebResourceRequest {
    // Prefer using other constructors over this one.
    public BvWebResourceRequest() {
    }

    public BvWebResourceRequest(String url, boolean isMainFrame, boolean hasUserGesture,
                                   String method, @Nullable HashMap<String, String> requestHeaders) {
        this.url = url;
        this.isMainFrame = isMainFrame;
        this.hasUserGesture = hasUserGesture;
        // Note: we intentionally let isRedirect default initialize to false. This is because we
        // don't always know if this request is associated with a redirect or not.
        this.method = method;
        this.requestHeaders = requestHeaders;
    }

    public BvWebResourceRequest(String url, boolean isMainFrame, boolean hasUserGesture,
                                   String method, @NonNull String[] requestHeaderNames,
                                   @NonNull String[] requestHeaderValues) {
        this(url, isMainFrame, hasUserGesture, method,
                new HashMap<String, String>(requestHeaderValues.length));
        for (int i = 0; i < requestHeaderNames.length; ++i) {
            this.requestHeaders.put(requestHeaderNames[i], requestHeaderValues[i]);
        }
    }

    // Url of the request.
    public String url;
    // Is this for the main frame or a child iframe?
    public boolean isMainFrame;
    // Was a gesture associated with the request? Don't trust can easily be spoofed.
    public boolean hasUserGesture;
    // Was it a result of a server-side redirect?
    public boolean isRedirect;
    // Method used (GET/POST/OPTIONS)
    public String method;
    // Headers that would have been sent to server.
    public HashMap<String, String> requestHeaders;
}
