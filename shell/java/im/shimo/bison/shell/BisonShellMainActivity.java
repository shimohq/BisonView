package im.shimo.bison.shell;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.graphics.Bitmap;
import android.text.TextUtils;
import android.net.http.SslError;
import android.util.Log;
import android.view.View;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.webkit.JavascriptInterface;

import org.chromium.base.CommandLine;
import org.chromium.content_public.browser.DeviceUtils;

import im.shimo.bison.BisonInitializer;
import im.shimo.bison.BisonView;
import im.shimo.bison.BisonViewClient;
import im.shimo.bison.BisonViewSettings;
import im.shimo.bison.WebResourceRequest;
import im.shimo.bison.BisonWebChromeClient;
import im.shimo.bison.SslErrorHandler;
import im.shimo.bison.GeolocationPermissions;
import im.shimo.bison.WebResourceResponse;
import im.shimo.bison.internal.BvSettings;

import java.net.MalformedURLException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;

public class BisonShellMainActivity extends Activity {

    private static final String TAG = "BisonShellMainActivity";
    private static final String COMMAND_LINE_ARGS_KEY = "commandLineArgs";
    private static final String INITIAL_URL = "https://www.baidu.com";

    private BisonView mBisonView;
    private EditText mUrlTextView;
    private ImageButton mPrevButton;
    private ImageButton mNextButton;

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        BisonInitializer.getInstance().ensureStarted();
        String[] commandLineParams = getCommandLineParamsFromIntent(getIntent());
        if (commandLineParams != null) {
            CommandLine.getInstance().appendSwitchesAndArguments(commandLineParams);
        }
        // TraceEvent.setATraceEnabled(true);
        DeviceUtils.addDeviceSpecificUserAgentSwitch();
        setContentView(R.layout.main_activity);
        initializeUrlField();
        initializeNavigationButtons();
        initializeBisonView();
        BisonView.setRemoteDebuggingEnabled(true);
        String startupUrl = getUrlFromIntent(getIntent());
        if (TextUtils.isEmpty(startupUrl)) {
            startupUrl = INITIAL_URL;
        }
        Log.i(TAG, "startup url :" + startupUrl);
        mBisonView.loadUrl(startupUrl);
        mUrlTextView.setText(startupUrl);
    }

    private void initializeUrlField() {
        mUrlTextView = (EditText) findViewById(R.id.url);
        mUrlTextView.setOnEditorActionListener((v, actionId, event) -> {
            if ((actionId != EditorInfo.IME_ACTION_GO)
                    && (event == null || event.getKeyCode() != KeyEvent.KEYCODE_ENTER
                            || event.getAction() != KeyEvent.ACTION_DOWN)) {
                return false;
            }

            String url = mUrlTextView.getText().toString();
            try {
                URI uri = new URI(url);
                if (uri.getScheme() == null) {
                    url = "http://" + uri.toString();
                } else {
                    url = uri.toString();
                }
            } catch (URISyntaxException e) {
                // Ignore syntax errors.
            }
            mBisonView.loadUrl(url);
            mUrlTextView.clearFocus();
            setKeyboardVisibilityForUrl(false);
            mBisonView.requestFocus();
            return true;
        });
        mUrlTextView.setOnFocusChangeListener((v, hasFocus) -> {
            setKeyboardVisibilityForUrl(hasFocus);
            mNextButton.setVisibility(hasFocus ? View.GONE : View.VISIBLE);
            mPrevButton.setVisibility(hasFocus ? View.GONE : View.VISIBLE);
            if (!hasFocus) {
                mUrlTextView.setText(mBisonView.getUrl());
            }
        });
    }

    private void initializeNavigationButtons() {
        mPrevButton = (ImageButton) findViewById(R.id.prev);
        mPrevButton.setOnClickListener(v -> {
            if (mBisonView.canGoBack()) {
                mBisonView.goBack();
            }
        });

        mNextButton = (ImageButton) findViewById(R.id.next);
        mNextButton.setOnClickListener(v -> {
            if (mBisonView.canGoForward()) {
                mBisonView.goForward();
            }
        });
    }

    private void initializeBisonView() {
        mBisonView = findViewById(R.id.bison_view);
        BisonViewSettings settings = mBisonView.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setAllowFileAccess(true);
        settings.setDatabaseEnabled(true);
        settings.setAllowUniversalAccessFromFileURLs(true);

        mBisonView.setBisonViewClient(new BisonViewClient() {

            @Override
            public void onPageStarted(BisonView view, String url, Bitmap favicon) {
                if (mUrlTextView != null) {
                    mUrlTextView.setText(url);
                }
            }

            @Override
            public boolean shouldOverrideUrlLoading(BisonView view, WebResourceRequest request) {
                return false;
            }

            @Override
            public void onReceivedSslError(BisonView view, SslErrorHandler handler,
                    SslError error) {
                Log.w(TAG, "onReceivedSslError");
                handler.proceed();
            }

            @Override
            public WebResourceResponse shouldInterceptRequest(BisonView view,
                    WebResourceRequest request) {
                return null;
            }

            @Override
            public void overrideRequest(BisonView view, WebResourceRequest request) {
                request.getRequestHeaders().put("Referer", "https://www.baidu.com");
            }

        });

        mBisonView.setBisonWebChromeClient(new BisonWebChromeClient() {

            @Override
            public void onReceivedTitle(BisonView view, String title) {
                Log.w(TAG, "onReceivedTitle : title = [" + title + "]");
            }

            @Override
            public void onGeolocationPermissionsShowPrompt(String origin,
                    GeolocationPermissions.Callback callback) {
                Log.w(TAG, "onGeolocationPermissionsShowPrompt");
            }

        });

        mBisonView.addJavascriptInterface(new Object() {

            @JavascriptInterface
            public void callMe() {
                Log.d(TAG, "callMe() called");
            }

        }, "test");

    }

    private void setKeyboardVisibilityForUrl(boolean visible) {
        InputMethodManager imm =
                (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        if (visible) {
            imm.showSoftInput(mUrlTextView, InputMethodManager.SHOW_IMPLICIT);
        } else {
            imm.hideSoftInputFromWindow(mUrlTextView.getWindowToken(), 0);
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
