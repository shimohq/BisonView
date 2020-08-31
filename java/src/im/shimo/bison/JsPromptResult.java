package im.shimo.bison;

public class JsPromptResult  extends JsResult {

    private String mStringResult;

    public void confirm(String result) {
        mStringResult = result;
        confirm();
    }


    public JsPromptResult(JsResult.ResultReceiver receiver) {
        super(receiver);
    }

    public String getStringResult() {
        return mStringResult;
    }
}
