package im.shimo.bisonview.test.util;

import android.app.Instrumentation;

import org.hamcrest.Matchers;
import org.junit.Assert;

import im.shimo.bison.internal.BvContents;

import org.chromium.base.test.util.Criteria;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.CriteriaNotSatisfiedException;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.test.util.TestCallbackHelperContainer.OnEvaluateJavaScriptResultHelper;
import org.chromium.content_public.browser.test.util.WebContentsUtils;

/**
 * Collection of functions for JavaScript-based interactions with a page.
 */
public class JSUtils {
    private static final long WAIT_TIMEOUT_MS = 2000L;
    private static final int CHECK_INTERVAL = 100;

    private static String createScriptToClickNode(String nodeId) {
        String script = "var evObj = new MouseEvent('click', {bubbles: true});"
                + "document.getElementById('" + nodeId + "').dispatchEvent(evObj);"
                + "console.log('element with id [" + nodeId + "] clicked');";
        return script;
    }

    public static void clickOnLinkUsingJs(final Instrumentation instrumentation,
            final BvContents bvContents,
            final OnEvaluateJavaScriptResultHelper onEvaluateJavaScriptResultHelper,
            final String linkId) {
        CriteriaHelper.pollInstrumentationThread(() -> {
            String linkIsNotNull = null;
            try {
                linkIsNotNull = executeJavaScriptAndWaitForResult(instrumentation, bvContents,
                        onEvaluateJavaScriptResultHelper,
                        "document.getElementById('" + linkId + "') != null");
            } catch (Throwable t) {
                throw new CriteriaNotSatisfiedException(t);
            }
            Criteria.checkThat(linkIsNotNull, Matchers.is("true"));
        }, WAIT_TIMEOUT_MS, CHECK_INTERVAL);

        // clang-format off
        instrumentation.runOnMainSync(
                () -> bvContents.getWebContents().evaluateJavaScriptForTests(
                        createScriptToClickNode(linkId), null));
        // clang-format on
    }

    public static void clickNodeWithUserGesture(WebContents webContents, String nodeId) {
        WebContentsUtils.evaluateJavaScriptWithUserGesture(
                webContents, createScriptToClickNode(nodeId), null);
    }

    public static String executeJavaScriptAndWaitForResult(Instrumentation instrumentation,
            final BvContents bvContents,
            final OnEvaluateJavaScriptResultHelper onEvaluateJavaScriptResultHelper,
            final String code) throws Exception {
        instrumentation.runOnMainSync(
                () -> onEvaluateJavaScriptResultHelper.evaluateJavaScriptForTests(
                        bvContents.getWebContents(), code));
        onEvaluateJavaScriptResultHelper.waitUntilHasValue();
        Assert.assertTrue("Failed to retrieve JavaScript evaluation results.",
                onEvaluateJavaScriptResultHelper.hasValue());
        return onEvaluateJavaScriptResultHelper.getJsonResultAndClear();
    }
}
