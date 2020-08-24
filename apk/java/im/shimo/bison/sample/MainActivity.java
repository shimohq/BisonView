package im.shimo.bison.sample;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.widget.Toast;

import org.chromium.base.CommandLine;
import org.chromium.base.MemoryPressureListener;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.content_public.browser.BrowserStartupController;
import org.chromium.content_public.browser.DeviceUtils;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.ActivityWindowAndroid;

import im.shimo.bison.BisonView;

public class MainActivity extends Activity {

    private static final String TAG = "MainActivity";

    private static final String ACTIVE_SHELL_URL_KEY = "activeUrl";
    public static final String COMMAND_LINE_ARGS_KEY = "commandLineArgs";

    // Native switch - shell_switches::kRunWebTests
    private static final String RUN_WEB_TESTS_SWITCH = "run-web-tests";

    // private BisonViewManager mBisonViewManager;
    private BisonView mBisonView;
    private ActivityWindowAndroid mWindowAndroid;
    private Intent mLastSentIntent;
    private String mStartupUrl;

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_activity);
        mBisonView = findViewById(R.id.bison_view);
        mBisonView.loadUrl("https://www.baidu.com");
    }

    // private void finishInitialization(Bundle savedInstanceState) {
    //     String url = "http://www.baidu.com";
        

    //     if (savedInstanceState != null
    //             && savedInstanceState.containsKey(ACTIVE_SHELL_URL_KEY)) {
    //         url = savedInstanceState.getString(ACTIVE_SHELL_URL_KEY);
    //     }
    //     mBisonView.init();
    //     mBisonView.loadUrl(url);
    //     // mBisonViewManager.launchShell();
    //     // mBisonViewManager.loadUrl(url);
    // }

    private void initializationFailed() {
        Log.e(TAG, "ContentView initialization failed.");
        Toast.makeText(this,
                R.string.browser_process_initialization_failed,
                Toast.LENGTH_SHORT).show();
        finish();
    }

    // @Override
    // protected void onSaveInstanceState(Bundle outState) {
    //     super.onSaveInstanceState(outState);
    //     WebContents webContents = getActiveWebContents();
    //     if (webContents != null) {
    //         outState.putString(ACTIVE_SHELL_URL_KEY, webContents.getLastCommittedUrl());
    //     }

    //     //mWindowAndroid.saveInstanceState(outState);
    // }

    // @Override
    // public boolean onKeyUp(int keyCode, KeyEvent event) {
    //     if (keyCode == KeyEvent.KEYCODE_BACK) {
    //         WebContents webContents = getActiveWebContents();
    //         if (webContents != null && webContents.getNavigationController().canGoBack()) {
    //             webContents.getNavigationController().goBack();
    //             return true;
    //         }
    //     }

    //     return super.onKeyUp(keyCode, event);
    // }

    // @Override
    // protected void onNewIntent(Intent intent) {
    //     if (getCommandLineParamsFromIntent(intent) != null) {
    //         Log.i(TAG, "Ignoring command line params: can only be set when creating the activity.");
    //     }

    //     if (MemoryPressureListener.handleDebugIntent(this, intent.getAction())) return;

    //     String url = getUrlFromIntent(intent);
    //     if (mBisonView!=null){
    //         mBisonView.loadUrl(url);
    //     }
    // }

    // @Override
    // protected void onStart() {
    //     super.onStart();

    //     WebContents webContents = getActiveWebContents();
    //     if (webContents != null) webContents.onShow();
    // }

    // @Override
    // public void onActivityResult(int requestCode, int resultCode, Intent data) {
    //     super.onActivityResult(requestCode, resultCode, data);
    //     mWindowAndroid.onActivityResult(requestCode, resultCode, data);
    // }

    // @Override
    // public void startActivity(Intent i) {
    //     mLastSentIntent = i;
    //     super.startActivity(i);
    // }

    // @Override
    // protected void onDestroy() {
    //     if (mBisonView != null) mBisonView.destroy();
    //     super.onDestroy();
    // }

    // public Intent getLastSentIntent() {
    //     return mLastSentIntent;
    // }

    // private static String getUrlFromIntent(Intent intent) {
    //     return intent != null ? intent.getDataString() : null;
    // }

    // private static String[] getCommandLineParamsFromIntent(Intent intent) {
    //     return intent != null ? intent.getStringArrayExtra(COMMAND_LINE_ARGS_KEY) : null;
    // }

    // /**
    //  * @return The {@link WebContents} owned by the currently visible {@link Shell} or null if
    //  *         one is not showing.
    //  */
    // public WebContents getActiveWebContents() {
    //     // BisonView bisonView = getActiveShell();
    //     return mBisonView != null ? mBisonView.getWebContents() : null;
    // }

}
