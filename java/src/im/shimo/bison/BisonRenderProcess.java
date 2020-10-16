
package im.shimo.bison;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

/**
 */
@JNINamespace("bison")
public final class BisonRenderProcess extends BisonSupportLibIsomorphic {
    private long mNativeRenderProcess;

    private BisonRenderProcess() {}

    public boolean terminate() {
        if (mNativeRenderProcess == 0) return false;

        return BisonRenderProcessJni.get().terminateChildProcess(
                mNativeRenderProcess, BisonRenderProcess.this);
    }

    @CalledByNative
    private static BisonRenderProcess create() {
        return new BisonRenderProcess();
    }

    @CalledByNative
    private void setNative(long nativeRenderProcess) {
        mNativeRenderProcess = nativeRenderProcess;
    }

    @NativeMethods
    interface Natives {
        boolean terminateChildProcess(long nativeBisonRenderProcess, BisonRenderProcess caller);
    }
}
