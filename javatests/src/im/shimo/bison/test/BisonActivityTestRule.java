package im.shimo.bison.test;

import static org.chromium.base.test.util.ScalableTimeout.scaleTimeout;

import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ActivityTestRule;
import android.util.AndroidRuntimeException;
import android.util.Base64;
import android.view.ViewGroup;

import org.junit.Assert;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

// import bison java's
import org.chromium.base.Log;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.InMemorySharedPreferences;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.test.util.CriteriaHelper;
import org.chromium.content_public.browser.test.util.TestCallbackHelperContainer.OnPageFinishedHelper;
import org.chromium.content_public.browser.test.util.TestThreadUtils;
import org.chromium.net.test.util.TestWebServer;

import java.lang.annotation.Annotation;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class BisonActivityTestRule extends ActivityTestRule<BisonActivityTestRule> {
    public static final long WAIT_TIMEOUT_MS = scaleTimeout(30000L);

    public static final int CHECK_INTERVAL = 100;

    private static final String TAG = "BisonActivityTestRule";

    private static final Pattern MAYBE_QUOTED_STRING = Pattern.compile("^(\"?)(.*)\\1$");

    private Description mCurrentTestDescription;

    public BisonActivityTestRule() {
        super(BisonActivityTestRule.class, /* initialTouchMode */ false, /* launchActivity */ false);
    }

    @Override
    public Statement apply(final Statement base, Description description) {
        mCurrentTestDescription = description;
        return super.apply(new Statement() {
            @Override
            public void evaluate() throws Throwable {
                setUp();
                base.evaluate();
                tearDown();
            }
        }, description);
    }

    public void setUp() {
        if (needsAwBrowserContextCreated()) {
            createAwBrowserContext();
        }
        if (needsBrowserProcessStarted()) {
            startBrowserProcess();
        }
    }

    public void tearDown() {
        if (!needsAwContentsCleanup()) return;

        TestThreadUtils.runOnUiThreadBlocking(() -> {
            for (WeakReference<AwContents> awContentsRef : mAwContentsDestroyedInTearDown) {
                AwContents awContents = awContentsRef.get();
                if (awContents == null) continue;
                awContents.destroy();
            }
        });
        // Flush the UI queue since destroy posts again to UI thread.
        TestThreadUtils.runOnUiThreadBlocking(() -> { mAwContentsDestroyedInTearDown.clear(); });
    }

    public AwTestRunnerActivity launchActivity() {
        if (getActivity() == null) {
            return launchActivity(null);
        }
        return getActivity();
    }


}
