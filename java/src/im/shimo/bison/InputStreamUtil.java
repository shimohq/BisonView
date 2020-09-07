package im.shimo.bison;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.io.IOException;
import java.io.InputStream;

@JNINamespace("bison")
class InputStreamUtil{

    private static final String LOGTAG = "InputStreamUtil";

    private static final int CALL_FAILED_STATUS = -1;
    private static final int EXCEPTION_THROWN_STATUS = -2;

    private static String logMessage(String method) {
        return "Got exception when calling " + method + "() on an InputStream returned from "
                + "shouldInterceptRequest. This will cause the related request to fail.";
    }

    @CalledByNative
    public static void close(InputStream stream) {
        try {
            stream.close();
        } catch (IOException e) {
            Log.e(LOGTAG, logMessage("close"), e);
        }
    }

    @CalledByNative
    public static int available(InputStream stream) {
        try {
            return Math.max(CALL_FAILED_STATUS, stream.available());
        } catch (IOException e) {
            Log.e(LOGTAG, logMessage("available"), e);
            return EXCEPTION_THROWN_STATUS;
        }
    }

    @CalledByNative
    public static int read(InputStream stream, byte[] b, int off, int len) {
        try {
            return Math.max(CALL_FAILED_STATUS, stream.read(b, off, len));
        } catch (IOException e) {
            Log.e(LOGTAG, logMessage("read"), e);
            return EXCEPTION_THROWN_STATUS;
        }
    }

    @CalledByNative
    public static long skip(InputStream stream, long n) {
        try {
            return Math.max(CALL_FAILED_STATUS, stream.skip(n));
        } catch (IOException e) {
            Log.e(LOGTAG, logMessage("skip"), e);
            return EXCEPTION_THROWN_STATUS;
        }
    }
}