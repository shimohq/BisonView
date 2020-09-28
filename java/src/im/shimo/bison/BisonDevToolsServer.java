package im.shimo.bison;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

/**
 * Controller for Remote Web Debugging (Developer Tools).
 */
@JNINamespace("bison")
class BisonDevToolsServer {

    private long mNativeDevToolsServer;

    public BisonDevToolsServer() {
        mNativeDevToolsServer =
                BisonDevToolsServerJni.get().initRemoteDebugging(BisonDevToolsServer.this);
    }

    public void destroy() {
        BisonDevToolsServerJni.get().destroyRemoteDebugging(
                BisonDevToolsServer.this, mNativeDevToolsServer);
        mNativeDevToolsServer = 0;
    }

    public void setRemoteDebuggingEnabled(boolean enabled) {
        BisonDevToolsServerJni.get().setRemoteDebuggingEnabled(
                BisonDevToolsServer.this, mNativeDevToolsServer, enabled);
    }

    @NativeMethods
    interface Natives {
        long initRemoteDebugging(BisonDevToolsServer caller);
        void destroyRemoteDebugging(BisonDevToolsServer caller, long devToolsServer);
        void setRemoteDebuggingEnabled(
                BisonDevToolsServer caller, long devToolsServer, boolean enabled);
    }
}