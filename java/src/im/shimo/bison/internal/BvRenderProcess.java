
package im.shimo.bison.internal;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
@JNINamespace("bison")
public final class BvRenderProcess  {
    private long mNativeRenderProcess;

    private BvRenderProcess() {}

    public boolean terminate() {
        if (mNativeRenderProcess == 0) return false;

        return BvRenderProcessJni.get().terminateChildProcess(
                mNativeRenderProcess, BvRenderProcess.this);
    }

    public boolean isProcessLockedToSiteForTesting() {
        if (mNativeRenderProcess == 0) return false;

        return BvRenderProcessJni.get().isProcessLockedToSiteForTesting(
                mNativeRenderProcess, BvRenderProcess.this);
    }

    @CalledByNative
    private static BvRenderProcess create() {
        return new BvRenderProcess();
    }

    @CalledByNative
    private void setNative(long nativeRenderProcess) {
        mNativeRenderProcess = nativeRenderProcess;
    }

    @NativeMethods
    interface Natives {
        boolean terminateChildProcess(long nativeBvRenderProcess, BvRenderProcess caller);
        boolean isProcessLockedToSiteForTesting(long nativeBvRenderProcess, BvRenderProcess caller);
    }
}
