package im.shimo.bison;

import androidx.annotation.RestrictTo;

public class JsPromptResult extends JsResult {

    private String mStringResult;

    public void confirm(String result) {
        mStringResult = result;
        confirm();
    }


    public JsPromptResult(JsResult.ResultReceiver receiver) {
        super(receiver);
    }

    @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
    public String getStringResult() {
        return mStringResult;
    }
    
}
