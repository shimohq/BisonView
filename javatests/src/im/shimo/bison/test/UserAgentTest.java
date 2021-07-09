package im.shimo.bison.test;

import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import im.shimo.bison.BisonContents;
import org.chromium.base.test.util.Feature;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

@RunWith(AwJUnit4ClassRunner.class)
public class UserAgentTest {

    @Rule 
    public BisonActivityTestRule mActivityTestRule = new AwActivityTestRule();

    @Before
    public void setUp() {
        mContentsClient = new TestAwContentsClient();
        mAwContents = mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient)
                              .getAwContents();
    }


}
