
package im.shimo.bisonview.test;

import android.annotation.SuppressLint;
import android.os.Build;
import android.os.LocaleList;
import android.support.test.InstrumentationRegistry;

import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import im.shimo.bison.internal.BvContents;
import im.shimo.bisonview.test.util.JSUtils;

import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.net.test.EmbeddedTestServer;

import java.util.Locale;
import java.util.regex.Pattern;

/**
 * Tests for Accept Language implementation.
 */
@RunWith(AwJUnit4ClassRunner.class)
public class AcceptLanguageTest {

    @Rule
    public BvActivityTestRule mActivityTestRule = new BvActivityTestRule();

    private TestBvContentsClient mContentsClient;
    private BvContents mBvContents;

    private EmbeddedTestServer mTestServer;

    @Before
    public void setUp() {
        mContentsClient = new TestBvContentsClient();
        mBvContents = mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient)
                              .getBvContents();

        mTestServer = EmbeddedTestServer.createAndStartServer(
                InstrumentationRegistry.getInstrumentation().getContext());
    }

    @After
    public void tearDown() {
        mTestServer.stopAndDestroyServer();
    }

    private static final Pattern COMMA_AND_OPTIONAL_Q_VALUE =
            Pattern.compile("(?:;q=[^,]+)?(?:,|$)");

    /**
     * Extract the languages from the Accept-Language header.
     *
     * The Accept-Language header can have more than one language along with optional quality
     * factors for each, e.g.
     *
     *  "de-DE,en-US;q=0.8,en-UK;q=0.5"
     *
     * This function extracts only the language strings from the Accept-Language header, so
     * the example above would yield the following:
     *
     *  ["de-DE", "en-US", "en-UK"]
     *
     * @param raw String containing the raw Accept-Language header
     * @return A list of languages as Strings.
     */
    private String[] getAcceptLanguages(String raw) {
        return COMMA_AND_OPTIONAL_Q_VALUE.split(mActivityTestRule.maybeStripDoubleQuotes(raw));
    }

    @SuppressLint("NewApi")
    private boolean isSingleLocale(String lang, String country) {
        String languageTag = String.format("%s-%s", lang, country);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            // In N+, multiple locales can be set.
            return languageTag.equals(LocaleList.getDefault().toLanguageTags());
        } else {
            return languageTag.equals(Locale.getDefault().toLanguageTag());
        }
    }

    @SuppressLint("NewApi")
    private void setSingleLocale(String lang, String country) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            LocaleList.setDefault(new LocaleList(new Locale(lang, country)));
        } else {
            Locale.setDefault(new Locale(lang, country));
        }
    }

    private void setLocaleForTesting(String lang, String country) {
        if (!isSingleLocale(lang, country)) {
            setSingleLocale(lang, country);
            mBvContents.updateDefaultLocale();
        }
    }

    /**
     * Verify that the Accept Language string is correct.
     */
    @Test
    @SmallTest
    @Feature({"BisonView"})
    public void testAcceptLanguage() throws Throwable {
        setLocaleForTesting("en", "US");

        mActivityTestRule.getAwSettingsOnUiThread(mBvContents).setJavaScriptEnabled(true);

        // This should yield a lightly formatted page with the contents of the Accept-Language
        // header, e.g. "en-US" or "de-DE,en-US;q=0.8", as the only text content.
        String url = mTestServer.getURL("/echoheader?Accept-Language");
        mActivityTestRule.loadUrlSync(mBvContents, mContentsClient.getOnPageFinishedHelper(), url);

        // Note that we extend the base language from language-region pair.
        String rawAcceptLanguages =
                mActivityTestRule.getJavaScriptResultBodyTextContent(mBvContents, mContentsClient);
        Assert.assertTrue("Accept-Language header should contain at least 1 q-value",
                rawAcceptLanguages.contains(";q="));
        String[] acceptLanguages = getAcceptLanguages(rawAcceptLanguages);
        Assert.assertArrayEquals(new String[] {"en-US", "en"}, acceptLanguages);

        // Our accept language list in user agent is different from navigator.languages, which is
        // fine.
        String[] acceptLanguagesJs = getAcceptLanguages(JSUtils.executeJavaScriptAndWaitForResult(
                InstrumentationRegistry.getInstrumentation(), mBvContents,
                mContentsClient.getOnEvaluateJavaScriptResultHelper(),
                "navigator.languages.join(',')"));
        Assert.assertArrayEquals(new String[] {"en-US"}, acceptLanguagesJs);

        // Test locale change at run time
        setLocaleForTesting("de", "DE");

        mActivityTestRule.loadUrlSync(mBvContents, mContentsClient.getOnPageFinishedHelper(), url);

        acceptLanguages = getAcceptLanguages(
                mActivityTestRule.getJavaScriptResultBodyTextContent(mBvContents, mContentsClient));
        // Note that we extend the base language from language-region pair, and we put en-US and en
        // at the end.
        Assert.assertArrayEquals(new String[] {"de-DE", "de", "en-US", "en"}, acceptLanguages);
    }

    /**
     * Verify that the Accept Languages string is correct.
     * When default locales do not contain "en-US" or "en-us",
     * "en-US" should be added with lowest priority.
     */
    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.N)
    @SuppressLint("NewApi")
    @Feature({"BisonView"})
    public void testAcceptLanguagesWithenUS() throws Throwable {
        LocaleList.setDefault(new LocaleList(new Locale("ko", "KR")));
        mBvContents.updateDefaultLocale();

        mActivityTestRule.getAwSettingsOnUiThread(mBvContents).setJavaScriptEnabled(true);

        // This should yield a lightly formatted page with the contents of the Accept-Language
        // header, e.g. "en-US,en" or "de-DE,de,en-US,en;q=0.8", as the only text content.
        String url = mTestServer.getURL("/echoheader?Accept-Language");
        mActivityTestRule.loadUrlSync(mBvContents, mContentsClient.getOnPageFinishedHelper(), url);

        // Note that we extend accept languages.
        Assert.assertArrayEquals(new String[] {"ko-KR", "ko", "en-US", "en"},
                getAcceptLanguages(mActivityTestRule.getJavaScriptResultBodyTextContent(
                        mBvContents, mContentsClient)));

        // Our accept language list in user agent is different from navigator.languages, which is
        // fine.
        String[] acceptLanguagesJs = getAcceptLanguages(JSUtils.executeJavaScriptAndWaitForResult(
                InstrumentationRegistry.getInstrumentation(), mBvContents,
                mContentsClient.getOnEvaluateJavaScriptResultHelper(),
                "navigator.languages.join(',')"));
        Assert.assertArrayEquals(new String[] {"ko-KR", "en-US"}, acceptLanguagesJs);

        // Test locales that contain "en-US" change at run time
        LocaleList.setDefault(new LocaleList(new Locale("de", "DE"), new Locale("en", "US")));
        mBvContents.updateDefaultLocale();

        mActivityTestRule.loadUrlSync(mBvContents, mContentsClient.getOnPageFinishedHelper(), url);

        // Note that we extend the base language from language-region pair.
        // Also, we put en-US at the lowest priority.
        Assert.assertArrayEquals(new String[] {"de-DE", "de", "en-US", "en"},
                getAcceptLanguages(mActivityTestRule.getJavaScriptResultBodyTextContent(
                        mBvContents, mContentsClient)));

        // Test locales that contain "en-us" change at run time
        LocaleList.setDefault(new LocaleList(new Locale("de", "DE"), new Locale("en", "us")));
        mBvContents.updateDefaultLocale();

        mActivityTestRule.loadUrlSync(mBvContents, mContentsClient.getOnPageFinishedHelper(), url);

        Assert.assertArrayEquals(new String[] {"de-DE", "de", "en-US", "en"},
                getAcceptLanguages(mActivityTestRule.getJavaScriptResultBodyTextContent(
                        mBvContents, mContentsClient)));

        // Test locales that do not contain "en-us" or "en-US" change at run time
        LocaleList.setDefault(new LocaleList(new Locale("de", "DE"), new Locale("ja", "JP")));
        mBvContents.updateDefaultLocale();

        mActivityTestRule.loadUrlSync(mBvContents, mContentsClient.getOnPageFinishedHelper(), url);

        Assert.assertArrayEquals(new String[] {"de-DE", "de", "ja-JP", "ja", "en-US", "en"},
                getAcceptLanguages(mActivityTestRule.getJavaScriptResultBodyTextContent(
                        mBvContents, mContentsClient)));
    }
}
