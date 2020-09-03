package im.shimo.bison;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.components.embedder_support.delegate.WebContentsDelegateAndroid;

@JNINamespace("bison")
public class BisonWebContentsDelegate extends WebContentsDelegateAndroid {

    private BisonContentsClient mContentsClient;

    public BisonWebContentsDelegate(BisonContentsClient bisonContentsClient) {
        mContentsClient = bisonContentsClient;
    }

    @Override
    public void onLoadProgressChanged(int progress) {
        //mContentsClient.getCallbackHelper().postOnProgressChanged(progress);
        mContentsClient.onProgressChanged(progress);
    }



}