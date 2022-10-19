package im.shimo.bison.test;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.LinearLayout;


import im.shimo.bison.BisonInitializer;

public class BvTestRunnerActivity extends Activity {

    private LinearLayout mLinearLayout;
    private Intent mLastSentIntent;
    private boolean mIgnoreStartActivity;

    @Override
    public void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        BisonInitializer.getInstance().ensureStarted();

        mLinearLayout = new LinearLayout(this);
        mLinearLayout.setOrientation(LinearLayout.VERTICAL);
        mLinearLayout.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT,
                LayoutParams.MATCH_PARENT));

        setContentView(mLinearLayout);
    }

    public int getRootLayoutWidth() {
        return mLinearLayout.getWidth();
    }


    public void addView(View view) {
        view.setLayoutParams(new LinearLayout.LayoutParams(
                LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT, 1f));
        mLinearLayout.addView(view);
    }

    /**
     * Clears the main linear layout.
     */
    public void removeAllViews() {
        mLinearLayout.removeAllViews();
    }

    @Override
    public void startActivity(Intent i) {
        mLastSentIntent = i;
        if (!mIgnoreStartActivity) super.startActivity(i);
    }

    public Intent getLastSentIntent() {
        return mLastSentIntent;
    }

    public void setIgnoreStartActivity(boolean ignore) {
        mIgnoreStartActivity = ignore;
    }

}
