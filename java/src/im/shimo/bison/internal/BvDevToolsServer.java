package im.shimo.bison.internal;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

import androidx.annotation.RestrictTo;

/**
 * Controller for Remote Web Debugging (Developer Tools).
 */
@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
@JNINamespace("bison")
public class BvDevToolsServer {

    private long mNativeDevToolsServer;

    public BvDevToolsServer() {
        mNativeDevToolsServer =
                BvDevToolsServerJni.get().initRemoteDebugging(BvDevToolsServer.this);
    }

    public void destroy() {
        BvDevToolsServerJni.get().destroyRemoteDebugging(
                BvDevToolsServer.this, mNativeDevToolsServer);
        mNativeDevToolsServer = 0;
    }

    public void setRemoteDebuggingEnabled(boolean enabled) {
        BvDevToolsServerJni.get().setRemoteDebuggingEnabled(
                BvDevToolsServer.this, mNativeDevToolsServer, enabled);
    }

    @NativeMethods
    interface Natives {
        long initRemoteDebugging(BvDevToolsServer caller);
        void destroyRemoteDebugging(BvDevToolsServer caller, long devToolsServer);
        void setRemoteDebuggingEnabled(
                BvDevToolsServer caller, long devToolsServer, boolean enabled);
    }
}
