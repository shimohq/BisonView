package im.shimo.bison.shell;

import android.app.Application;
import android.content.Context;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.PathUtils;
import org.chromium.base.multidex.ChromiumMultiDexInstaller;
import org.chromium.ui.base.ResourceBundle;

import im.shimo.bison.BisonInitializer;

public class BisonShellApp extends Application {

    @Override
    public void onCreate() {
      super.onCreate();
      BisonInitializer.getInstance().init(this);
      BisonInitializer.initCommandLine();
    }

}
