package im.shimo.bison.internal;

import androidx.annotation.VisibleForTesting;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * A helper class for WebView-specific handling of Java crashes.
 */
@JNINamespace("bison")
public class BvCrashReporterClient {
    // The filename prefix used by Chromium proguarding, which we use to
    // recognise stack frames that reference WebView.
    private static final String CHROMIUM_PREFIX = "chromium-";

    /**
     * Determine if a Throwable should be reported to the crash reporting mechanism.
     *
     *
     * @param t The throwable to check.
     * @return True if any frame of the stack trace matches the above rule.
     */
    @VisibleForTesting
    @CalledByNative
    public static boolean stackTraceContainsWebViewCode(Throwable t) {
        for (StackTraceElement frame : t.getStackTrace()) {
            if (frame.getClassName().startsWith("im.shimo.bison.")
                    || frame.getClassName().startsWith("org.chromium.")
                    || (frame.getFileName() != null
                            && frame.getFileName().startsWith(CHROMIUM_PREFIX))) {
                return true;
            }
        }
        return false;
    }
}
