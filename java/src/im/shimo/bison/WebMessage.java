package im.shimo.bison;

import androidx.annotation.Nullable;

public class WebMessage {

    private String mData;
    private WebMessagePort[] mPorts;

    /**
     * Creates a WebMessage.
     * @param data  the data of the message.
     */
    public WebMessage(String data) {
        mData = data;
    }

    /**
     * Creates a WebMessage.
     * @param data  the data of the message.
     * @param ports  the ports that are sent with the message.
     */
    public WebMessage(String data, WebMessagePort[] ports) {
        mData = data;
        mPorts = ports;
    }

    /**
     * Returns the data of the message.
     */
    public String getData() {
        return mData;
    }

    /**
     * Returns the ports that are sent with the message, or {@code null} if no port
     * is sent.
     */
    @Nullable
    public WebMessagePort[] getPorts() {
        return mPorts;
    }
}
