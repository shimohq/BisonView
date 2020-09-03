package im.shimo.bison;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

@JNINamespace("bison")
public class BisonWebContentsDelegate  {

    @CalledByNative
    private void onLoadProgressChanged(double progress) {
        
    }



}