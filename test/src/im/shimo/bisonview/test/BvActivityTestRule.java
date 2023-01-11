// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bisonview.test;

import android.content.Context;
import android.content.Intent;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.lifecycle.Stage;
import android.util.AndroidRuntimeException;
import android.util.Base64;
import android.view.ViewGroup;

import org.junit.Assert;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import im.shimo.bison.internal.*;
import im.shimo.bison.test.BvTestRunnerActivity;
import im.shimo.bisonview.test.util.GraphicsTestUtils;
import im.shimo.bisonview.test.util.JSUtils;

import org.chromium.base.Log;
import org.chromium.base.test.BaseActivityTestRule;
import org.chromium.base.test.util.ApplicationTestUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.InMemorySharedPreferences;
import org.chromium.base.test.util.ScalableTimeout;
import org.chromium.content_public.browser.LoadUrlParams;
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

/** Custom ActivityTestRunner for WebView instrumentation tests */
public class BvActivityTestRule extends BaseActivityTestRule<BvTestRunnerActivity> {
    public static final long WAIT_TIMEOUT_MS = 15000L;

    // Only use scaled timeout if you are certain it's not being called further up the call stack.
    public static final long SCALED_WAIT_TIMEOUT_MS = ScalableTimeout.scaleTimeout(15000L);

    public static final int CHECK_INTERVAL = 100;

    private static final String TAG = "BvActivityTestRule";

    private static final Pattern MAYBE_QUOTED_STRING = Pattern.compile("^(\"?)(.*)\\1$");

    private static boolean sBrowserProcessStarted;

    /**
     * An interface to call onCreateWindow(BvContents).
     */
    public interface OnCreateWindowHandler {
        /** This will be called when a new window pops up from the current webview. */
        public boolean onCreateWindow(BvContents bvContents);
    }

    private Description mCurrentTestDescription;

    // The browser context needs to be a process-wide singleton.
    private BvBrowserContext mBrowserContext;

    private List<WeakReference<BvContents>> mBvContentsDestroyedInTearDown = new ArrayList<>();

    public BvActivityTestRule() {
        super(BvTestRunnerActivity.class);
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
        if (needsBvBrowserContextCreated()) {
            createBvBrowserContext();
        }
        if (needsBrowserProcessStarted()) {
            startBrowserProcess();
        } else {
            assert !sBrowserProcessStarted
                : "needsBrowserProcessStarted false and @Batch are incompatible";
        }
    }

    public void tearDown() {
        if (!needsBvContentsCleanup()) return;

        TestThreadUtils.runOnUiThreadBlocking(() -> {
            for (WeakReference<BvContents> bvContentsRef : mBvContentsDestroyedInTearDown) {
                BvContents bvContents = bvContentsRef.get();
                if (bvContents == null) continue;
                bvContents.destroy();
            }
        });
        // Flush the UI queue since destroy posts again to UI thread.
        TestThreadUtils.runOnUiThreadBlocking(() -> { mBvContentsDestroyedInTearDown.clear(); });
    }

    public boolean needsHideActionBar() {
        return false;
    }

    private Intent getLaunchIntent() {
        if (needsHideActionBar()) {
            Intent intent = getActivityIntent();
            intent.putExtra(BvTestRunnerActivity.FLAG_HIDE_ACTION_BAR, true);
            return intent;
        }
        return null;
    }

    @Override
    public void launchActivity(Intent intent) {
        if (getActivity() != null) return;
        super.launchActivity(intent);
        ApplicationTestUtils.waitForActivityState(getActivity(), Stage.RESUMED);
    }

    public BvTestRunnerActivity launchActivity() {
        launchActivity(getLaunchIntent());
        return getActivity();
    }

    public BvBrowserContext createBvBrowserContextOnUiThread(InMemorySharedPreferences prefs) {
        // Native pointer is initialized later in startBrowserProcess if needed.
        return new BvBrowserContext(prefs, 0, true);
    }

    public TestDependencyFactory createTestDependencyFactory() {
        return new TestDependencyFactory();
    }

    /**
     * Override this to return false if the test doesn't want to create an
     * BvBrowserContext automatically.
     */
    public boolean needsBvBrowserContextCreated() {
        return true;
    }

    /**
     * Override this to return false if the test doesn't want the browser
     * startup sequence to be run automatically.
     *
     * @return Whether the instrumentation test requires the browser process to
     *         already be started.
     */
    public boolean needsBrowserProcessStarted() {
        return true;
    }

    /**
     * Override this to return false if test doesn't need all BvContents to be
     * destroyed explicitly after the test.
     */
    public boolean needsBvContentsCleanup() {
        return true;
    }

    public void createBvBrowserContext() {
        if (mBrowserContext != null) {
            throw new AndroidRuntimeException("There should only be one browser context.");
        }
        launchActivity(); // The Activity must be launched in order to load native code
        final InMemorySharedPreferences prefs = new InMemorySharedPreferences();
        TestThreadUtils.runOnUiThreadBlockingNoException(
                () -> mBrowserContext = createBvBrowserContextOnUiThread(prefs));
    }

    public void startBrowserProcess() {
        doStartBrowserProcess(false);
    }

    public void startBrowserProcessWithVulkan() {
        doStartBrowserProcess(true);
    }

    private void doStartBrowserProcess(boolean useVulkan) {
        // The Activity must be launched in order for proper webview statics to be setup.
        launchActivity();
        if (!sBrowserProcessStarted) {
            sBrowserProcessStarted = true;
            TestThreadUtils.runOnUiThreadBlocking(() -> {
                AwTestContainerView.installDrawFnFunctionTable(useVulkan);
                AwBrowserProcess.start();
            });
        }
        if (mBrowserContext != null) {
            TestThreadUtils.runOnUiThreadBlocking(
                    () -> mBrowserContext.setNativePointer(
                            BvBrowserContext.getDefault().getNativePointer()));
        }
    }

    public void runOnUiThread(Runnable r) {
        TestThreadUtils.runOnUiThreadBlocking(r);
    }

    public static void enableJavaScriptOnUiThread(final BvContents bvContents) {
        TestThreadUtils.runOnUiThreadBlocking(
                () -> bvContents.getSettings().setJavaScriptEnabled(true));
    }

    public static boolean getJavaScriptEnabledOnUiThread(final BvContents bvContents)
            throws ExecutionException {
        return TestThreadUtils.runOnUiThreadBlocking(
                () -> bvContents.getSettings().getJavaScriptEnabled());
    }

    public static void setNetworkAvailableOnUiThread(
            final BvContents bvContents, final boolean networkUp) {
        TestThreadUtils.runOnUiThreadBlocking(() -> bvContents.setNetworkAvailable(networkUp));
    }

    /**
     * Loads url on the UI thread and blocks until onPageFinished is called.
     */
    public void loadUrlSync(final BvContents bvContents, CallbackHelper onPageFinishedHelper,
            final String url) throws Exception {
        loadUrlSync(bvContents, onPageFinishedHelper, url, null);
    }

    public void loadUrlSync(final BvContents bvContents, CallbackHelper onPageFinishedHelper,
            final String url, final Map<String, String> extraHeaders) throws Exception {
        int currentCallCount = onPageFinishedHelper.getCallCount();
        loadUrlAsync(bvContents, url, extraHeaders);
        onPageFinishedHelper.waitForCallback(
                currentCallCount, 1, WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
    }

    public void loadUrlSyncAndExpectError(final BvContents bvContents,
            CallbackHelper onPageFinishedHelper, CallbackHelper onReceivedErrorHelper,
            final String url) throws Exception {
        int onReceivedErrorCount = onReceivedErrorHelper.getCallCount();
        int onFinishedCallCount = onPageFinishedHelper.getCallCount();
        loadUrlAsync(bvContents, url);
        onReceivedErrorHelper.waitForCallback(
                onReceivedErrorCount, 1, WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
        onPageFinishedHelper.waitForCallback(
                onFinishedCallCount, 1, WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
    }

    /**
     * Loads url on the UI thread but does not block.
     */
    public void loadUrlAsync(final BvContents bvContents, final String url) {
        loadUrlAsync(bvContents, url, null);
    }

    public void loadUrlAsync(
            final BvContents bvContents, final String url, final Map<String, String> extraHeaders) {
        TestThreadUtils.runOnUiThreadBlocking(() -> bvContents.loadUrl(url, extraHeaders));
    }

    /**
     * Posts url on the UI thread and blocks until onPageFinished is called.
     */
    public void postUrlSync(final BvContents bvContents, CallbackHelper onPageFinishedHelper,
            final String url, byte[] postData) throws Exception {
        int currentCallCount = onPageFinishedHelper.getCallCount();
        postUrlAsync(bvContents, url, postData);
        onPageFinishedHelper.waitForCallback(
                currentCallCount, 1, WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
    }

    /**
     * Loads url on the UI thread but does not block.
     */
    public void postUrlAsync(final BvContents bvContents, final String url, byte[] postData) {
        class PostUrl implements Runnable {
            byte[] mPostData;
            public PostUrl(byte[] postData) {
                mPostData = postData;
            }
            @Override
            public void run() {
                bvContents.postUrl(url, mPostData);
            }
        }
        TestThreadUtils.runOnUiThreadBlocking(new PostUrl(postData));
    }

    /**
     * Loads data on the UI thread and blocks until onPageFinished is called.
     */
    public void loadDataSync(final BvContents bvContents, CallbackHelper onPageFinishedHelper,
            final String data, final String mimeType, final boolean isBase64Encoded)
            throws Exception {
        int currentCallCount = onPageFinishedHelper.getCallCount();
        loadDataAsync(bvContents, data, mimeType, isBase64Encoded);
        onPageFinishedHelper.waitForCallback(
                currentCallCount, 1, WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
    }

    public void loadHtmlSync(final BvContents bvContents, CallbackHelper onPageFinishedHelper,
            final String html) throws Throwable {
        int currentCallCount = onPageFinishedHelper.getCallCount();
        final String encodedData = Base64.encodeToString(html.getBytes(), Base64.NO_PADDING);
        loadDataSync(bvContents, onPageFinishedHelper, encodedData, "text/html", true);
    }

    public void loadDataSyncWithCharset(final BvContents bvContents,
            CallbackHelper onPageFinishedHelper, final String data, final String mimeType,
            final boolean isBase64Encoded, final String charset) throws Exception {
        int currentCallCount = onPageFinishedHelper.getCallCount();
        TestThreadUtils.runOnUiThreadBlocking(
                () -> bvContents.loadUrl(LoadUrlParams.createLoadDataParams(
                                data, mimeType, isBase64Encoded, charset)));
        onPageFinishedHelper.waitForCallback(
                currentCallCount, 1, WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
    }

    /**
     * Loads data on the UI thread but does not block.
     */
    public void loadDataAsync(final BvContents bvContents, final String data, final String mimeType,
            final boolean isBase64Encoded) {
        TestThreadUtils.runOnUiThreadBlocking(
                () -> bvContents.loadData(data, mimeType, isBase64Encoded ? "base64" : null));
    }

    public void loadDataWithBaseUrlSync(final BvContents bvContents,
            CallbackHelper onPageFinishedHelper, final String data, final String mimeType,
            final boolean isBase64Encoded, final String baseUrl, final String historyUrl)
            throws Throwable {
        int currentCallCount = onPageFinishedHelper.getCallCount();
        loadDataWithBaseUrlAsync(bvContents, data, mimeType, isBase64Encoded, baseUrl, historyUrl);
        onPageFinishedHelper.waitForCallback(
                currentCallCount, 1, WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
    }

    public void loadDataWithBaseUrlAsync(final BvContents bvContents, final String data,
            final String mimeType, final boolean isBase64Encoded, final String baseUrl,
            final String historyUrl) throws Throwable {
        runOnUiThread(() -> bvContents.loadDataWithBaseURL(baseUrl, data, mimeType,
                                      isBase64Encoded ? "base64" : null, historyUrl));
    }

    /**
     * Reloads the current page synchronously.
     */
    public void reloadSync(final BvContents bvContents, CallbackHelper onPageFinishedHelper)
            throws Exception {
        int currentCallCount = onPageFinishedHelper.getCallCount();
        TestThreadUtils.runOnUiThreadBlocking(
                () -> bvContents.getNavigationController().reload(true));
        onPageFinishedHelper.waitForCallback(
                currentCallCount, 1, WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
    }

    /**
     * Stops loading on the UI thread.
     */
    public void stopLoading(final BvContents bvContents) {
        TestThreadUtils.runOnUiThreadBlocking(() -> bvContents.stopLoading());
    }

    public void waitForVisualStateCallback(final BvContents bvContents) throws Exception {
        final CallbackHelper ch = new CallbackHelper();
        final int chCount = ch.getCallCount();
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            final long requestId = 666;
            bvContents.insertVisualStateCallback(requestId, new BvContents.VisualStateCallback() {
                @Override
                public void onComplete(long id) {
                    Assert.assertEquals(requestId, id);
                    ch.notifyCalled();
                }
            });
        });
        ch.waitForCallback(chCount);
    }

    public void insertVisualStateCallbackOnUIThread(final BvContents bvContents,
            final long requestId, final BvContents.VisualStateCallback callback) {
        TestThreadUtils.runOnUiThreadBlocking(
                () -> bvContents.insertVisualStateCallback(requestId, callback));
    }

    // Waits for the pixel at the center of BvContents to color up into expectedColor.
    // Note that this is a stricter condition that waiting for a visual state callback,
    // as visual state callback only indicates that *something* has appeared in WebView.
    public void waitForPixelColorAtCenterOfView(final BvContents bvContents,
            final AwTestContainerView testContainerView, final int expectedColor) {
        pollUiThread(() -> GraphicsTestUtils.getPixelColorAtCenterOfView(
                    bvContents, testContainerView) == expectedColor);
    }

    public AwTestContainerView createAwTestContainerView(final BvContentsClient bvContentsClient) {
        return createAwTestContainerView(bvContentsClient, false, null);
    }

    public AwTestContainerView createAwTestContainerView(final BvContentsClient bvContentsClient,
            boolean supportsLegacyQuirks, final TestDependencyFactory testDependencyFactory) {
        AwTestContainerView testContainerView = createDetachedAwTestContainerView(
                bvContentsClient, supportsLegacyQuirks, testDependencyFactory);
        getActivity().addView(testContainerView);
        testContainerView.requestFocus();
        return testContainerView;
    }

    public BvBrowserContext getBvBrowserContext() {
        return mBrowserContext;
    }

    public AwTestContainerView createDetachedAwTestContainerView(
            final BvContentsClient bvContentsClient) {
        return createDetachedAwTestContainerView(bvContentsClient, false, null);
    }

    public AwTestContainerView createDetachedAwTestContainerView(
            final BvContentsClient bvContentsClient, boolean supportsLegacyQuirks,
            TestDependencyFactory testDependencyFactory) {
        if (testDependencyFactory == null) {
            testDependencyFactory = createTestDependencyFactory();
        }
        boolean allowHardwareAcceleration = isHardwareAcceleratedTest();
        final AwTestContainerView testContainerView =
                testDependencyFactory.createAwTestContainerView(
                        getActivity(), allowHardwareAcceleration);

        BvSettings awSettings =
                testDependencyFactory.createBvSettings(getActivity(), supportsLegacyQuirks);
        BvContents bvContents = testDependencyFactory.createBvContents(mBrowserContext,
                testContainerView, testContainerView.getContext(),
                testContainerView.getInternalAccessDelegate(),
                testContainerView.getNativeDrawFunctorFactory(), bvContentsClient, awSettings,
                testDependencyFactory);
        testContainerView.initialize(bvContents);
        mBvContentsDestroyedInTearDown.add(new WeakReference<>(bvContents));
        return testContainerView;
    }

    public boolean isHardwareAcceleratedTest() {
        return !testMethodHasAnnotation(DisableHardwareAccelerationForTest.class);
    }

    public AwTestContainerView createAwTestContainerViewOnMainSync(final BvContentsClient client) {
        return createAwTestContainerViewOnMainSync(client, false, null);
    }

    public AwTestContainerView createAwTestContainerViewOnMainSync(
            final BvContentsClient client, final boolean supportsLegacyQuirks) {
        return createAwTestContainerViewOnMainSync(client, supportsLegacyQuirks, null);
    }

    public AwTestContainerView createAwTestContainerViewOnMainSync(final BvContentsClient client,
            final boolean supportsLegacyQuirks, final TestDependencyFactory testDependencyFactory) {
        return TestThreadUtils.runOnUiThreadBlockingNoException(
                () -> createAwTestContainerView(
                                client, supportsLegacyQuirks, testDependencyFactory));
    }

    public void destroyBvContentsOnMainSync(final BvContents bvContents) {
        if (bvContents == null) return;
        TestThreadUtils.runOnUiThreadBlocking(() -> bvContents.destroy());
    }

    public String getTitleOnUiThread(final BvContents bvContents) throws Exception {
        return TestThreadUtils.runOnUiThreadBlocking(() -> bvContents.getTitle());
    }

    public BvSettings getBvSettingsOnUiThread(final BvContents bvContents) throws Exception {
        return TestThreadUtils.runOnUiThreadBlocking(() -> bvContents.getSettings());
    }

    /**
     * Verify double quotes in both sides of the raw string. Strip the double quotes and
     * returns rest of the string.
     */
    public String maybeStripDoubleQuotes(String raw) {
        Assert.assertNotNull(raw);
        Matcher m = MAYBE_QUOTED_STRING.matcher(raw);
        Assert.assertTrue(m.matches());
        return m.group(2);
    }

    /**
     * Executes the given snippet of JavaScript code within the given ContentView. Returns the
     * result of its execution in JSON format.
     */
    public String executeJavaScriptAndWaitForResult(final BvContents bvContents,
            TestBvContentsClient viewClient, final String code) throws Exception {
        return executeJavaScriptAndWaitForResult(
                bvContents, viewClient, code, /*shouldCheckSettings=*/true);
    }

    /**
     * Like {@link #executeJavaScriptAndWaitForResult} but with a parameter to skip the call to
     * {@link checkJavaScriptEnabled}. This is useful if your test expects JavaScript to be disabled
     * (in which case the underlying executeJavaScriptAndWaitForResult() is expected to be a NOOP).
     */
    public String executeJavaScriptAndWaitForResult(final BvContents bvContents,
            TestBvContentsClient viewClient, final String code, boolean shouldCheckSettings)
            throws Exception {
        if (shouldCheckSettings) {
            checkJavaScriptEnabled(bvContents);
        }
        return JSUtils.executeJavaScriptAndWaitForResult(
                InstrumentationRegistry.getInstrumentation(), bvContents,
                viewClient.getOnEvaluateJavaScriptResultHelper(), code);
    }

    public static void checkJavaScriptEnabled(BvContents bvContents) throws Exception {
        boolean javaScriptEnabled = BvActivityTestRule.getJavaScriptEnabledOnUiThread(bvContents);
        if (!javaScriptEnabled) {
            throw new IllegalStateException(
                    "JavaScript is disabled in this BvContents; did you forget to call "
                    + "BvActivityTestRule.enableJavaScriptOnUiThread()?");
        }
    }

    /**
     * Executes JavaScript code within the given ContentView to get the text content in
     * document body. Returns the result string without double quotes.
     */
    public String getJavaScriptResultBodyTextContent(
            final BvContents bvContents, final TestBvContentsClient viewClient) throws Exception {
        String raw = executeJavaScriptAndWaitForResult(
                bvContents, viewClient, "document.body.textContent");
        return maybeStripDoubleQuotes(raw);
    }

    /**
     * Adds a JavaScript interface to the BvContents. Does its work synchronously on the UI thread,
     * and can be called from any thread. All the rules of {@link
     * android.webkit.WebView#addJavascriptInterface} apply to this method (ex. you must call this
     * <b>prior</b> to loading the frame you intend to load the JavaScript interface into).
     *
     * @param bvContents the BvContents in which to insert the JavaScript interface.
     * @param objectToInject the JavaScript interface to inject.
     * @param javascriptIdentifier the name with which to refer to {@code objectToInject} from
     *        JavaScript code.
     */
    public static void addJavascriptInterfaceOnUiThread(final BvContents bvContents,
            final Object objectToInject, final String javascriptIdentifier) throws Exception {
        checkJavaScriptEnabled(bvContents);
        TestThreadUtils.runOnUiThreadBlocking(
                () -> bvContents.addJavascriptInterface(objectToInject, javascriptIdentifier));
    }

    /**
     * Wrapper around CriteriaHelper.pollInstrumentationThread. This uses BvActivityTestRule-specifc
     * timeouts and treats timeouts and exceptions as test failures automatically.
     */
    public static void pollInstrumentationThread(final Callable<Boolean> callable) {
        CriteriaHelper.pollInstrumentationThread(() -> {
            try {
                return callable.call();
            } catch (Throwable e) {
                Log.e(TAG, "Exception while polling.", e);
                return false;
            }
        }, WAIT_TIMEOUT_MS, CHECK_INTERVAL);
    }

    /**
     * Wrapper around {@link BvActivityTestRule#pollInstrumentationThread()} but runs the
     * callable on the UI thread.
     */
    public void pollUiThread(final Callable<Boolean> callable) {
        pollInstrumentationThread(() -> TestThreadUtils.runOnUiThreadBlocking(callable));
    }

    /**
     * Waits for {@code future} and returns its value (or times out). If {@code future} has an
     * associated Exception, this will re-throw that Exception on the instrumentation thread
     * (wrapping with an unchecked Exception if necessary, to avoid requiring callers to declare
     * checked Exceptions).
     *
     * @param future the {@link Future} representing a value of interest.
     * @return the value {@code future} represents.
     */
    public static <T> T waitForFuture(Future<T> future) {
        try {
            return future.get(SCALED_WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
        } catch (ExecutionException e) {
            // ExecutionException means this Future has an associated Exception that we should
            // re-throw on the current thread. We throw the cause instead of ExecutionException,
            // since ExecutionException itself isn't interesting, and might mislead those debugging
            // test failures to suspect this method is the culprit (whereas the root cause is from
            // another thread).
            Throwable cause = e.getCause();
            // If the cause is an unchecked Throwable type, re-throw as-is.
            if (cause instanceof Error) throw(Error) cause;
            if (cause instanceof RuntimeException) throw(RuntimeException) cause;
            // Otherwise, wrap this in an unchecked Exception so callers don't need to declare
            // checked Exceptions.
            throw new RuntimeException(cause);
        } catch (InterruptedException | TimeoutException e) {
            // Don't call e.getCause() for either of these. Unlike ExecutionException, these don't
            // wrap the root cause, but rather are themselves interesting. Again, we wrap these
            // checked Exceptions with an unchecked Exception for the caller's convenience.
            //
            // Although we might be tempted to handle InterruptedException by calling
            // Thread.currentThread().interrupt(), this is not correct in this case. The interrupted
            // thread was likely a different thread than the current thread, so there's nothing
            // special we need to do.
            throw new RuntimeException(e);
        }
    }

    /**
     * Takes an element out of the {@link BlockingQueue} (or times out).
     */
    public static <T> T waitForNextQueueElement(BlockingQueue<T> queue) throws Exception {
        T value = queue.poll(SCALED_WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
        if (value == null) {
            // {@code null} is the special value which means {@link BlockingQueue#poll} has timed
            // out (also: there's no risk for collision with real values, because BlockingQueue does
            // not allow null entries). Instead of returning this special value, let's throw a
            // proper TimeoutException.
            throw new TimeoutException(
                    "Timeout while trying to take next entry from BlockingQueue");
        }
        return value;
    }

    /**
     * Clears the resource cache. Note that the cache is per-application, so this will clear the
     * cache for all WebViews used.
     */
    public void clearCacheOnUiThread(final BvContents bvContents, final boolean includeDiskFiles) {
        TestThreadUtils.runOnUiThreadBlocking(() -> bvContents.clearCache(includeDiskFiles));
    }

    /**
     * Returns pure page scale.
     */
    public float getScaleOnUiThread(final BvContents bvContents) throws Exception {
        return TestThreadUtils.runOnUiThreadBlocking(() -> bvContents.getPageScaleFactor());
    }

    /**
     * Returns page scale multiplied by the screen density.
     */
    public float getPixelScaleOnUiThread(final BvContents bvContents) throws Exception {
        return TestThreadUtils.runOnUiThreadBlocking(() -> bvContents.getScale());
    }

    /**
     * Returns whether a user can zoom the page in.
     */
    public boolean canZoomInOnUiThread(final BvContents bvContents) throws Exception {
        return TestThreadUtils.runOnUiThreadBlocking(() -> bvContents.canZoomIn());
    }

    /**
     * Returns whether a user can zoom the page out.
     */
    public boolean canZoomOutOnUiThread(final BvContents bvContents) throws Exception {
        return TestThreadUtils.runOnUiThreadBlocking(() -> bvContents.canZoomOut());
    }

    /**
     * Loads the main html then triggers the popup window.
     */
    public void triggerPopup(final BvContents parentBvContents,
            TestBvContentsClient parentBvContentsClient, TestWebServer testWebServer,
            String mainHtml, String popupHtml, String popupPath, String triggerScript)
            throws Exception {
        enableJavaScriptOnUiThread(parentBvContents);
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            parentBvContents.getSettings().setSupportMultipleWindows(true);
            parentBvContents.getSettings().setJavaScriptCanOpenWindowsAutomatically(true);
        });

        final String parentUrl = testWebServer.setResponse("/popupParent.html", mainHtml, null);
        if (popupHtml != null) {
            testWebServer.setResponse(popupPath, popupHtml, null);
        } else {
            testWebServer.setResponseWithNoContentStatus(popupPath);
        }

        parentBvContentsClient.getOnCreateWindowHelper().setReturnValue(true);
        loadUrlSync(parentBvContents, parentBvContentsClient.getOnPageFinishedHelper(), parentUrl);

        TestBvContentsClient.OnCreateWindowHelper onCreateWindowHelper =
                parentBvContentsClient.getOnCreateWindowHelper();
        int currentCallCount = onCreateWindowHelper.getCallCount();
        TestThreadUtils.runOnUiThreadBlocking(
                () -> parentBvContents.evaluateJavaScriptForTests(triggerScript, null));
        onCreateWindowHelper.waitForCallback(
                currentCallCount, 1, WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
    }

    /**
     * Supplies the popup window with BvContents then waits for the popup window to finish loading.
     * @param parentBvContents Parent webview's BvContents.
     */
    public PopupInfo connectPendingPopup(BvContents parentBvContents) throws Exception {
        PopupInfo popupInfo = createPopupContents(parentBvContents);
        loadPopupContents(parentBvContents, popupInfo, null);
        return popupInfo;
    }

    /**
     * Creates a popup window with BvContents.
     */
    public PopupInfo createPopupContents(final BvContents parentBvContents) {
        TestBvContentsClient popupContentsClient;
        AwTestContainerView popupContainerView;
        final BvContents popupContents;
        popupContentsClient = new TestBvContentsClient();
        popupContainerView = createAwTestContainerViewOnMainSync(popupContentsClient);
        popupContents = popupContainerView.getBvContents();
        enableJavaScriptOnUiThread(popupContents);
        return new PopupInfo(popupContentsClient, popupContainerView, popupContents);
    }

    /**
     * Waits for the popup window to finish loading.
     * @param parentBvContents Parent webview's BvContents.
     * @param info The PopupInfo.
     * @param onCreateWindowHandler An instance of OnCreateWindowHandler. null if there isn't.
     */
    public void loadPopupContents(final BvContents parentBvContents, PopupInfo info,
            OnCreateWindowHandler onCreateWindowHandler) throws Exception {
        TestBvContentsClient popupContentsClient = info.popupContentsClient;
        AwTestContainerView popupContainerView = info.popupContainerView;
        final BvContents popupContents = info.popupContents;
        OnPageFinishedHelper onPageFinishedHelper = popupContentsClient.getOnPageFinishedHelper();
        int finishCallCount = onPageFinishedHelper.getCallCount();

        if (onCreateWindowHandler != null) onCreateWindowHandler.onCreateWindow(popupContents);

        TestBvContentsClient.OnReceivedTitleHelper onReceivedTitleHelper =
                popupContentsClient.getOnReceivedTitleHelper();
        int titleCallCount = onReceivedTitleHelper.getCallCount();

        TestThreadUtils.runOnUiThreadBlocking(
                () -> parentBvContents.supplyContentsForPopup(popupContents));

        onPageFinishedHelper.waitForCallback(
                finishCallCount, 1, WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
        onReceivedTitleHelper.waitForCallback(
                titleCallCount, 1, WAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
    }

    private boolean testMethodHasAnnotation(Class<? extends Annotation> clazz) {
        return mCurrentTestDescription.getAnnotation(clazz) != null ? true : false;
    }

    /**
     * Factory class used in creation of test BvContents instances. Test cases
     * can provide subclass instances to the createAwTest* methods in order to
     * create an BvContents instance with injected test dependencies.
     */
    public static class TestDependencyFactory extends BvContents.DependencyFactory {
        public AwTestContainerView createAwTestContainerView(
                BvTestRunnerActivity activity, boolean allowHardwareAcceleration) {
            return new AwTestContainerView(activity, allowHardwareAcceleration);
        }

        public BvSettings createBvSettings(Context context, boolean supportsLegacyQuirks) {
            return new BvSettings(context, false /* isAccessFromFileURLsGrantedByDefault */,
                    supportsLegacyQuirks, false /* allowEmptyDocumentPersistence */,
                    true /* allowGeolocationOnInsecureOrigins */,
                    false /* doNotUpdateSelectionOnMutatingSelectionRange */);
        }

        public BvContents createBvContents(BvBrowserContext browserContext, ViewGroup containerView,
                Context context, InternalAccessDelegate internalAccessAdapter,
                NativeDrawFunctorFactory nativeDrawFunctorFactory, BvContentsClient contentsClient,
                BvSettings settings, DependencyFactory dependencyFactory) {
            return new BvContents(browserContext, containerView, context, internalAccessAdapter,
                    nativeDrawFunctorFactory, contentsClient, settings, dependencyFactory);
        }
    }

    /**
     * POD object for holding references to helper objects of a popup window.
     */
    public static class PopupInfo {
        public final TestBvContentsClient popupContentsClient;
        public final AwTestContainerView popupContainerView;
        public final BvContents popupContents;

        public PopupInfo(TestBvContentsClient popupContentsClient,
                AwTestContainerView popupContainerView, BvContents popupContents) {
            this.popupContentsClient = popupContentsClient;
            this.popupContainerView = popupContainerView;
            this.popupContents = popupContents;
        }
    }
}
