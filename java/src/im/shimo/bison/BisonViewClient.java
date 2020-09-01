package im.shimo.bison;

import android.graphics.Bitmap;

public class BisonViewClient {

    public boolean shouldOverrideUrlLoading(BisonView view, WebResourceRequest request) {
        return false;
    }


    public void onPageStarted(BisonView view, String url, Bitmap favicon){

    }



}