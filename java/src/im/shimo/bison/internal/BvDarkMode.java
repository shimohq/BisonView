
package im.shimo.bison.internal;

import android.content.Context;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.content_public.browser.WebContents;

/**
 * The class to handle dark mode.
 */
@JNINamespace("bison")
public class BvDarkMode {
    private static Boolean sAppTargetsTForTesting;
    private Context mContext;
    private long mNativeBvDarkMode;

    private static boolean sEnableSimplifiedDarkMode;

    public static void enableSimplifiedDarkMode() {
        sEnableSimplifiedDarkMode = true;
        BvDarkModeJni.get().enableSimplifiedDarkMode();
    }

    public BvDarkMode(Context context) {
        mContext = context;
    }

    public void setWebContents(WebContents webContents) {
        if (mNativeBvDarkMode != 0) {
            BvDarkModeJni.get().detachFromJavaObject(mNativeBvDarkMode, this);
            mNativeBvDarkMode = 0;
        }
        if (webContents != null) {
            mNativeBvDarkMode = BvDarkModeJni.get().init(this, webContents);
        }
    }

    public static boolean isSimplifiedDarkModeEnabled() {
        return sEnableSimplifiedDarkMode;
    }

    public void destroy() {
        setWebContents(null);
    }

    @CalledByNative
    private boolean isAppUsingDarkTheme() {
        return DarkModeHelper.LightTheme.LIGHT_THEME_FALSE
                == DarkModeHelper.getLightTheme(mContext);
    }

    @CalledByNative
    private void onNativeObjectDestroyed() {
        mNativeBvDarkMode = 0;
    }

    @NativeMethods
    interface Natives {
        void enableSimplifiedDarkMode();
        long init(BvDarkMode caller, WebContents webContents);
        void detachFromJavaObject(long nativeBvDarkMode, BvDarkMode caller);
    }
}
