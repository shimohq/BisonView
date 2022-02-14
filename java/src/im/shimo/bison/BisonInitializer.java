package im.shimo.bison;

import android.app.Application;
import android.content.Context;
import android.os.Build;
import android.os.StrictMode;
import android.text.TextUtils;

import im.shimo.bison.adapter.BvWebStorageAdapter;
import im.shimo.bison.adapter.CookieManagerAdapter;
import im.shimo.bison.adapter.GeolocationPermissionsAdapter;
import im.shimo.bison.internal.BvBrowserContext;
import im.shimo.bison.internal.BvCookieManager;
import im.shimo.bison.internal.BvRunQueue;

import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.PathUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.content_public.browser.BrowserStartupController;
import org.chromium.content_public.browser.ChildProcessCreationParams;
import org.chromium.ui.base.ResourceBundle;

import java.lang.reflect.Field;

import androidx.annotation.Nullable;

public class BisonInitializer {

    public static final String PRIVATE_DATA_DIRECTORY_SUFFIX = "bisonview";

    private static volatile boolean mStarted;

    private static volatile BisonInitializer INSTANCE;

    private final BvRunQueue mRunQueue = new BvRunQueue(() -> {
        return BisonInitializer.this.isStarted();
    });
    final Object mLock = new Object();
    private BvBrowserContext mBrowserContext;
    private GeolocationPermissionsAdapter mGeolocationPermissions;
    private BisonViewWebStorage mWebStorage;
    private CookieManager mCookieManager;

    private final String mAppPackageName;
    private Boolean mIsHostAppDebug;

    public static BisonInitializer getInstance() {
        if (INSTANCE == null) {
            synchronized (BisonInitializer.class) {
                if (INSTANCE == null) {
                    INSTANCE = new BisonInitializer();
                }
            }
        }
        return INSTANCE;
    }

    private BisonInitializer() {

        final Context context = ContextUtils.getApplicationContext();
        final boolean isExternalService = false;
        final boolean bindToCaller = true;
        final boolean ignoreVisibilityForImportance = true;
        mAppPackageName = context.getPackageName();
        PathUtils.setPrivateDataDirectorySuffix(PRIVATE_DATA_DIRECTORY_SUFFIX);
        ResourceBundle.setAvailablePakLocales(new String[] {}, BvLocaleConfig.getWebViewSupportedPakLocales());
        ChildProcessCreationParams.set(mAppPackageName, "im.shimo.bison.PrivilegedProcessService", mAppPackageName,
                "im.shimo.bison.SandboxedProcessService", isExternalService, LibraryProcessType.PROCESS_WEBVIEW_CHILD,
                bindToCaller, ignoreVisibilityForImportance);
        BisonResources.resetIds(context);
    }

    public void ensureStarted() {
        synchronized (mLock) {
            startLocked();
        }
    }

    public void startLocked() {
        assert Thread.holdsLock(mLock) && ThreadUtils.runningOnUiThread();
        mLock.notifyAll();
        if (mStarted) {
            return;
        }
        LibraryLoader.getInstance().setLibraryProcessType(LibraryProcessType.PROCESS_WEBVIEW);
        LibraryLoader.getInstance().loadNow();
        BrowserStartupController.getInstance().startBrowserProcessesSync(LibraryProcessType.PROCESS_WEBVIEW, false);

        mBrowserContext = BvBrowserContext.getDefault();
        mGeolocationPermissions = new GeolocationPermissionsAdapter(mBrowserContext.getGeolocationPermissions());
        mWebStorage = new BvWebStorageAdapter(mBrowserContext.getQuotaManagerBridge());

        mStarted = true;
    }

    public boolean isStarted() {
        return mStarted;
    }

    public BvRunQueue getRunQueue() {
        return mRunQueue;
    }

    public BvBrowserContext getBrowserContext() {
        ensureStarted();
        return mBrowserContext;
    }

    public CookieManager getCookieManager() {
        ensureStarted();
        if (mCookieManager == null) {
            mCookieManager = new CookieManagerAdapter(new BvCookieManager());
        }
        return mCookieManager;
    }

    public BisonViewWebStorage getWebStorage() {
        return mWebStorage;
    }

    public GeolocationPermissions getGeolocationPermissions() {
        return mGeolocationPermissions;
    }

    public void initCommandLine(@Nullable String path) {
        if (IsHostAppDebug() && !TextUtils.isEmpty(path)) {
            StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
            CommandLine.initFromFile(path);
            StrictMode.setThreadPolicy(oldPolicy);
        } else {
            CommandLine.init(null);
        }
    }

    private boolean IsHostAppDebug() {
        if (mIsHostAppDebug != null) {
            return mIsHostAppDebug;
        }
        try {
            Class<?> buildConfigClass = Class.forName(String.format("%s.BuildConfig", mAppPackageName));
            Field debugField = buildConfigClass.getDeclaredField("DEBUG");
            debugField.setAccessible(true);
            mIsHostAppDebug = (Boolean) debugField.get(null);
            return mIsHostAppDebug;
        } catch (ClassNotFoundException | NoSuchFieldException | IllegalAccessException
                | IllegalArgumentException ignore) {
            return false;
        }
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
                return (String) activityThreadClazz.getMethod("currentProcessName").invoke(null);
            } catch (Exception var1) {
                throw new RuntimeException(var1);
            }
        }
    }

}
