package im.shimo.bison.internal;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

import androidx.annotation.RestrictTo;

/**
 * Exposes a subset of Chromium form database to BisonView database for managing autocomplete
 * functionality.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
@JNINamespace("bison")
public class BvFormDatabase {

    public static boolean hasFormData() {
        return BvFormDatabaseJni.get().hasFormData();
    }

    public static void clearFormData() {
        BvFormDatabaseJni.get().clearFormData();
    }

    @NativeMethods
    interface Natives {
        boolean hasFormData();
        void clearFormData();
    }
}
