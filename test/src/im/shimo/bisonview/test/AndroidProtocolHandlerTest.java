package im.shimo.bisonview.test;

import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import im.shimo.bison.internal.AndroidProtocolHandler;

import org.chromium.base.FileUtils;
import org.chromium.base.test.util.Feature;
import org.chromium.url.GURL;

import java.io.IOException;
import java.io.InputStream;

/**
 * Test AndroidProtocolHandler.
 */
@RunWith(BvJUnit4ClassRunner.class)
public class AndroidProtocolHandlerTest {
    @Rule
    public BvActivityTestRule mActivityTestRule = new BvActivityTestRule();

    @Test
    @SmallTest
    @Feature({"BisonView"})
    public void testOpenNullUrl() {
        Assert.assertNull(AndroidProtocolHandler.open(null));
    }

    @Test
    @SmallTest
    @Feature({"BisonView"})
    public void testOpenEmptyUrl() {
        Assert.assertNull(AndroidProtocolHandler.open(GURL.emptyGURL()));
    }

    @Test
    @SmallTest
    @Feature({"BisonView"})
    public void testOpenMalformedUrl() {
        Assert.assertNull(AndroidProtocolHandler.open(new GURL("abcdefg")));
    }

    @Test
    @SmallTest
    @Feature({"BisonView"})
    public void testOpenPathlessUrl() {
        // These URLs are interesting because android.net.Uri parses them unintuitively:
        // Uri.getPath() returns "/" but Uri.getLastPathSegment() returns null.
        Assert.assertNull(AndroidProtocolHandler.open(new GURL("file:///")));
        Assert.assertNull(AndroidProtocolHandler.open(new GURL("content:///")));
    }

    // star.svg and star.svgz contain the same data. AndroidProtocolHandler should decompress the
    // svgz automatically. Load both from assets and assert that they're equal.
    @Test
    @SmallTest
    @Feature({"BisonView"})
    public void testSvgzAsset() throws IOException {
        InputStream svgStream = null;
        InputStream svgzStream = null;
        try {
            svgStream = assertOpen(new GURL("file:///android_asset/star.svg"));
            byte[] expectedData = FileUtils.readStream(svgStream);

            svgzStream = assertOpen(new GURL("file:///android_asset/star.svgz"));
            byte[] actualData = FileUtils.readStream(svgzStream);

            Assert.assertArrayEquals(
                    "Decompressed star.svgz doesn't match star.svg", expectedData, actualData);
        } finally {
            if (svgStream != null) svgStream.close();
            if (svgzStream != null) svgzStream.close();
        }
    }

    private InputStream assertOpen(GURL url) {
        InputStream stream = AndroidProtocolHandler.open(url);
        Assert.assertNotNull("Failed top open \"" + url.getPossiblyInvalidSpec() + "\"", stream);
        return stream;
    }
}
