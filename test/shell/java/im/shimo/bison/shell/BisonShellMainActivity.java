package im.shimo.bison.shell;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.net.http.SslError;
import android.util.Log;

import org.chromium.base.CommandLine;
import org.chromium.content_public.browser.DeviceUtils;

import im.shimo.bison.BisonView;
import im.shimo.bison.BisonViewClient;
import im.shimo.bison.WebResourceRequest;
import im.shimo.bison.SslErrorHandler;

public class BisonShellMainActivity extends Activity {

    private static final String TAG = "BisonShellMainActivity";

    public static final String COMMAND_LINE_ARGS_KEY = "commandLineArgs";

    private BisonView mBisonView;

    private String mStartupUrl;

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        String[] commandLineParams = getCommandLineParamsFromIntent(getIntent());
        if (commandLineParams != null) {
            CommandLine.getInstance().appendSwitchesAndArguments(commandLineParams);
        }

        DeviceUtils.addDeviceSpecificUserAgentSwitch();

        setContentView(R.layout.main_activity);
        mBisonView = findViewById(R.id.bison_view);
        BisonView.setRemoteDebuggingEnabled(true);
        mBisonView.getSettings().setJavaScriptEnabled(true);
        mBisonView.setBisonViewClient(new BisonViewClient() {

            @Override
            public boolean shouldOverrideUrlLoading(BisonView view, WebResourceRequest request) {
                return false;
            }

            @Override
            public void onReceivedSslError(BisonView view, SslErrorHandler handler,
                                   SslError error) {
                //super.onReceivedSslError(view, handler, error);
                Log.w("MainActivity","onReceivedSslError");
                handler.proceed();
            }

        });

        mStartupUrl = getUrlFromIntent(getIntent());
        if (!TextUtils.isEmpty(mStartupUrl)) {
            mBisonView.loadUrl(mStartupUrl);
        } else {
            mBisonView.loadUrl("https://www.baidu.com");
        }
    }




    private static String getUrlFromIntent(Intent intent) {
        return intent != null ? intent.getDataString() : null;
    }

    private static String[] getCommandLineParamsFromIntent(Intent intent) {
        return intent != null ? intent.getStringArrayExtra(COMMAND_LINE_ARGS_KEY) : null;
    }

    @Override
    public void onBackPressed() {
        if (mBisonView.canGoBack()) {
            mBisonView.goBack();
        } else {
            super.onBackPressed();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mBisonView != null) {
            mBisonView.destroy();
            mBisonView = null;
        }
    }

}
