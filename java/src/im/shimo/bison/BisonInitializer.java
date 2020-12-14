package im.shimo.bison;

import android.app.Application;
import android.content.Context;
import android.os.Build;
import android.os.StrictMode;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.BuildInfo;
import org.chromium.base.CommandLine;
import org.chromium.base.PathUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.content_public.browser.BrowserStartupController;
import org.chromium.content_public.browser.ChildProcessCreationParams;
import org.chromium.ui.base.ResourceBundle;


public class BisonInitializer {

    public static final String PRIVATE_DATA_DIRECTORY_SUFFIX = "bison";

    private BisonInitializer() {
    }

    private static class SingletonInstance {
        private static final BisonInitializer INSTANCE = new BisonInitializer();
    }

    public static BisonInitializer getInstance() {
        return SingletonInstance.INSTANCE;
    }

    public void init(Context context) {
        ContextUtils.initApplicationContext(context);
        ResourceBundle.setNoAvailableLocalePaks();
        final boolean isExternalService = false;
        final boolean bindToCaller = true;
        final boolean ignoreVisibilityForImportance = true;
        ChildProcessCreationParams.set(context.getPackageName(),  "im.shimo.bison.PrivilegedProcessService",
                context.getPackageName(), "im.shimo.bison.SandboxedProcessService", isExternalService,
                LibraryProcessType.PROCESS_WEBVIEW_CHILD, bindToCaller,
                ignoreVisibilityForImportance);   
        if (isBrowserProcess()) {
            PathUtils.setPrivateDataDirectorySuffix(PRIVATE_DATA_DIRECTORY_SUFFIX);
            ApplicationStatus.initialize((Application) context.getApplicationContext());
            BisonResources.resetIds(context);
        }
        initCommandLine();
    }

    public static boolean isBrowserProcess() {
        return !getProcessName().contains(":");
    }

    public static String getProcessName() {
        if (Build.VERSION.SDK_INT >= 28) {
            return Application.getProcessName();
        } else {
            try {
                Class<?> activityThreadClazz = Class.forName("android.app.ActivityThread");
                return (String)activityThreadClazz.getMethod("currentProcessName").invoke(null);
            } catch (Exception var1) {
                throw new RuntimeException(var1);
            }
        }
    }



    static void initCommandLine() {
        if (BuildInfo.isDebugAndroid()) {
            StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
            CommandLine.initFromFile("/data/local/tmp/bison-view-command-line");
            StrictMode.setThreadPolicy(oldPolicy);
        } else {
            CommandLine.init(null);
        }
    }




}