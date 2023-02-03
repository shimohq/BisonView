package im.shimo.bison;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Build;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.os.StrictMode;

import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.PathUtils;
import org.chromium.base.PowerMonitor;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.metrics.ScopedSysTraceEvent;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskRunner;
import org.chromium.base.task.TaskTraits;
import org.chromium.content_public.browser.BrowserStartupController;
import org.chromium.content_public.browser.ChildProcessCreationParams;
import org.chromium.content_public.browser.ChildProcessLauncherHelper;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.channels.FileLock;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public final class BvBrowserProcess {
    private static final String TAG = "BvBrowserProcess";

    private static final String WEBVIEW_DIR_BASENAME = "bison";
    private static final String EXCLUSIVE_LOCK_FILE = "bison_data.lock";

    // To avoid any potential synchronization issues we post all minidump-copying
    // actions to
    // the same sequence to be run serially.
    private static final TaskRunner sSequencedTaskRunner = PostTask
            .createSequencedTaskRunner(TaskTraits.BEST_EFFORT_MAY_BLOCK);

    private static RandomAccessFile sLockFile;
    private static FileLock sExclusiveFileLock;
    private static String sWebViewPackageName;

    /**
     * Loads the native library, and performs basic static construction of objects
     * needed
     * to run webview in this process. Does not create threads; safe to call from
     * zygote.
     * Note: it is up to the caller to ensure this is only called once.
     *
     * @param processDataDirSuffix The suffix to use when setting the data directory
     *                             for this
     *                             process; null to use no suffix.
     */
    public static void loadLibrary(String processDataDirSuffix) {
        LibraryLoader.getInstance().setLibraryProcessType(LibraryProcessType.PROCESS_WEBVIEW);
        if (processDataDirSuffix == null) {
            PathUtils.setPrivateDataDirectorySuffix(WEBVIEW_DIR_BASENAME, "BisonView");
        } else {
            String processDataDirName = WEBVIEW_DIR_BASENAME + "_" + processDataDirSuffix;
            PathUtils.setPrivateDataDirectorySuffix(processDataDirName, processDataDirName);
        }
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        try {
            LibraryLoader.getInstance().loadNow();
            // Switch the command line implementation from Java to native.
            // It's okay for the WebView to do this before initialization because we have
            // setup the JNI bindings by this point.
            LibraryLoader.getInstance().switchCommandLineForWebView();
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }
    }

    /**
     * Configures child process launcher. This is required only if child services are used in
     * WebView.
     */
    public static void configureChildProcessLauncher() {
        final boolean isExternalService = true;
        final boolean bindToCaller = true;
        final boolean ignoreVisibilityForImportance = true;
        ChildProcessCreationParams.set(mAppPackageName, "im.shimo.bison.PrivilegedProcessService",
                mAppPackageName, "im.shimo.bison.SandboxedProcessService", isExternalService,
                LibraryProcessType.PROCESS_WEBVIEW_CHILD, bindToCaller,
                ignoreVisibilityForImportance);
    }

    /**
     * Starts the chromium browser process running within this process. Creates
     * threads
     * and performs other per-app resource allocations; must not be called from
     * zygote.
     * Note: it is up to the caller to ensure this is only called once.
     */
    @SuppressWarnings("deprecation")
    public static void start() {
        try (ScopedSysTraceEvent e1 = ScopedSysTraceEvent.scoped("BvBrowserProcess.start")) {
            final Context appContext = ContextUtils.getApplicationContext();
            // We must post to the UI thread to cover the case that the user
            // has invoked Chromium startup by using the (thread-safe)
            // CookieManager rather than creating a WebView.
            //noinspection deprecation
            ThreadUtils.runOnUiThreadBlocking(() -> {
                boolean multiProcess = CommandLine.getInstance().hasSwitch(BisonSwitches.WEBVIEW_SANDBOXED_RENDERER);
                if (multiProcess) {
                    ChildProcessLauncherHelper.warmUp(appContext, true);
                }
                // The policies are used by browser startup, so we need to register the policy
                // providers before starting the browser process. This only registers java objects
                // and doesn't need the native library.

                CombinedPolicyProvider.get().registerProvider(new BisonPolicyProvider(appContext));

                try (ScopedSysTraceEvent e2 = ScopedSysTraceEvent.scoped(
                        "BvBrowserProcess.startBrowserProcessesSync")) {
                    BrowserStartupController.getInstance().startBrowserProcessesSync(
                            LibraryProcessType.PROCESS_WEBVIEW, !multiProcess);
                }

                PowerMonitor.create();
            });
        }
    }

    public static void setWebViewPackageName(String webViewPackageName) {
        assert sWebViewPackageName == null || sWebViewPackageName.equals(webViewPackageName);
        sWebViewPackageName = webViewPackageName;
    }

    public static String getWebViewPackageName() {
        if (sWebViewPackageName == null) return ""; // May be null in testing.
        return sWebViewPackageName;
    }

    // Do not instantiate this class.
    private BvBrowserProcess() {
    }
}
