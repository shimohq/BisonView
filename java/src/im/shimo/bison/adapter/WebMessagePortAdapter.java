package im.shimo.bison.adapter;

import android.os.Handler;

import im.shimo.bison.WebMessage;
import im.shimo.bison.WebMessagePort;

import org.chromium.content_public.browser.MessagePort;

public class WebMessagePortAdapter extends WebMessagePort {

    private MessagePort mPort;

    public WebMessagePortAdapter(MessagePort port) {
        mPort = port;
    }

    @Override
    public void postMessage(WebMessage message) {
        mPort.postMessage(message.getData(), toMessagePorts(message.getPorts()));
    }

    @Override
    public void close() {
        mPort.close();
    }

    @Override
    public void setWebMessageCallback(WebMessageCallback callback) {
        setWebMessageCallback(callback, null);
    }

    @Override
    public void setWebMessageCallback(final WebMessageCallback callback, final Handler handler) {
        mPort.setMessageCallback(new MessagePort.MessageCallback() {
            @Override
            public void onMessage(String message, MessagePort[] ports) {
                callback.onMessage(WebMessagePortAdapter.this, new WebMessage(message, fromMessagePorts(ports)));
            }
        }, handler);
    }

    public MessagePort getPort() {
        return mPort;
    }

    public static WebMessagePort[] fromMessagePorts(MessagePort[] messagePorts) {
        if (messagePorts == null)
            return null;
        WebMessagePort[] ports = new WebMessagePort[messagePorts.length];
        for (int i = 0; i < messagePorts.length; i++) {
            ports[i] = new WebMessagePortAdapter(messagePorts[i]);
        }
        return ports;
    }

    public static MessagePort[] toMessagePorts(WebMessagePort[] webMessagePorts) {
        if (webMessagePorts == null)
            return null;
        MessagePort[] ports = new MessagePort[webMessagePorts.length];
        for (int i = 0; i < webMessagePorts.length; i++) {
            ports[i] = ((WebMessagePortAdapter) webMessagePorts[i]).getPort();
        }
        return ports;
    }

}
