package im.shimo.bison;

import android.os.Handler;

import androidx.annotation.RestrictTo;

public abstract class WebMessagePort {

    public static abstract class WebMessageCallback {
        /**
         * Message callback for receiving onMessage events.
         *
         * @param port    the WebMessagePort that the message is destined for
         * @param message the message from the entangled port.
         */
        public void onMessage(WebMessagePort port, WebMessage message) {
        }
    }

    @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
    public WebMessagePort() {
    }

    public abstract void postMessage(WebMessage message);

    /**
     * Close the message port and free any resources associated with it.
     */
    public abstract void close();

    /**
     * Sets a callback to receive message events on the main thread.
     *
     * @param callback the message callback.
     */
    public abstract void setWebMessageCallback(WebMessageCallback callback);

    /**
     * Sets a callback to receive message events on the handler that is provided by
     * the application.
     *
     * @param callback the message callback.
     * @param handler  the handler to receive the message messages.
     */
    public abstract void setWebMessageCallback(WebMessageCallback callback, Handler handler);
}
