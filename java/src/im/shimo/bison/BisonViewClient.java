package im.shimo.bison;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import android.graphics.Bitmap;
import android.net.http.SslError;
import android.os.Message;
import android.view.InputEvent;
import android.view.KeyEvent;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class BisonViewClient {

    private static Method gViewRootImplMethod;
    private static Method gDispatchUnhandledInputEventMethod;

    public boolean shouldOverrideUrlLoading(BisonView view, WebResourceRequest request) {
        return false;
    }


    public void onPageStarted(BisonView view, String url, Bitmap favicon) {

    }


    public void onPageFinished(BisonView view, String url) {

    }

    public void onLoadResource(BisonView view, String url) {
    }

    public void onPageCommitVisible(BisonView view, String url) {
    }

    public WebResourceResponse shouldInterceptRequest(BisonView view, WebResourceRequest request) {
        return null;
    }

    public void overrideRequest(BisonView view, WebResourceRequest request) {

    }

    public void onReceivedError(BisonView view, int errorCode,
            String description, String failingUrl) {
    }

    public void onReceivedError(BisonView view, WebResourceRequest request, WebResourceError error) {

    }

    public void onReceivedHttpError(
            BisonView view, WebResourceRequest request, WebResourceResponse errorResponse) {
    }

    /**
     * As the host application if the browser should resend data as the
     * requested page was a result of a POST. The default is to not resend the
     * data.
     *
     * @param view The BisonView that is initiating the callback.
     * @param dontResend The message to send if the browser should not resend
     * @param resend The message to send if the browser should resend data
     */
    public void onFormResubmission(BisonView view, Message dontResend,
            Message resend) {
        dontResend.sendToTarget();
    }

    /**
     * Notify the host application to update its visited links database.
     * @param view The BisonView that is initiating the callback.
     * @param url The url being visited.
     * @param isReload  {@code true} if this url is being reloaded.
     */
    public void doUpdateVisitedHistory(BisonView view, String url, boolean isReload) {
    }

    public void onReceivedSslError(BisonView view, SslErrorHandler handler,
                                   SslError error) {
        handler.cancel();
    }

    public void onReceivedClientCertRequest(BisonView view, ClientCertRequest request) {
        request.cancel();
    }

    /**
     * Notifies the host application that the WebView received an HTTP
     * authentication request. The host application can use the supplied
     * {@link HttpAuthHandler} to set the WebView's response to the request.
     * The default behavior is to cancel the request.
     *
     * @param view the BisonView that is initiating the callback
     * @param handler the HttpAuthHandler used to set the WebView's response
     * @param host the host requiring authentication
     * @param realm the realm for which authentication is required
     * @see BisonView#getHttpAuthUsernamePassword
     */
    public void onReceivedHttpAuthRequest(BisonView view,
            HttpAuthHandler handler, String host, String realm) {
        handler.cancel();
    }

    public boolean shouldOverrideKeyEvent(BisonView view, KeyEvent event) {
        return false;
    }

    public void onUnhandledKeyEvent(BisonView view, KeyEvent event) {
        onUnhandledInputEventInternal(view, event);
    }

    public void onUnhandledInputEvent(BisonView view, InputEvent event) {
        if (event instanceof KeyEvent) {
            onUnhandledKeyEvent(view, (KeyEvent) event);
            return;
        }
        onUnhandledInputEventInternal(view, event);
    }

    private void onUnhandledInputEventInternal(BisonView view, InputEvent event) {
        try {
            Object root = getViewRootImplMethod().invoke(view);
            if (root != null) {
                getDispatchUnhandledInputEventMethod().invoke(root, event);
            }
        } catch (NoSuchMethodException | IllegalAccessException
                | InvocationTargetException | ClassNotFoundException e) {
            e.printStackTrace();
        }
    }

    public void onScaleChanged(BisonView view, float oldScale, float newScale) {
    }

    public void onReceivedLoginRequest(BisonView view, String realm,
        @Nullable String account, String args) {
    }

    @NonNull
    private static Method getViewRootImplMethod() throws NoSuchMethodException {
        if (gViewRootImplMethod == null) {
            gViewRootImplMethod = View.class.getDeclaredMethod("getViewRootImpl");
        }
        return gViewRootImplMethod;
    }

    @NonNull
    private Method getDispatchUnhandledInputEventMethod()
            throws ClassNotFoundException, NoSuchMethodException {
        Class<?> clazz = Class.forName("android.view.ViewRootImpl");
        gDispatchUnhandledInputEventMethod = clazz
                .getDeclaredMethod("dispatchUnhandledInputEvent", InputEvent.class);
        return gDispatchUnhandledInputEventMethod;
    }

}
