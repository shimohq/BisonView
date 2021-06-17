package im.shimo.bison.sample;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;

import im.shimo.bison.BisonView;
import im.shimo.bison.BisonViewClient;
import im.shimo.bison.WebResourceRequest;

public class MainActivity extends Activity {

    private static final String TAG = "MainActivity";

    public static final String COMMAND_LINE_ARGS_KEY = "commandLineArgs";

    private BisonView mBisonView;

    private String mStartupUrl;

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_activity);
        mBisonView = findViewById(R.id.bison_view);

        mBisonView.getSettings().setJavaScriptEnabled(true);
        mBisonView.setBisonViewClient(new BisonViewClient() {

            @Override
            public boolean shouldOverrideUrlLoading(BisonView view, WebResourceRequest request) {
                return false;
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
