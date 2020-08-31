package im.shimo.bison;

public class JsResult {

    public interface ResultReceiver {
        void onJsResultComplete(JsResult result);
    }


    private final ResultReceiver mReceiver;

    private boolean mResult;


    public final void cancel() {
        mResult = false;
        wakeUp();
    }


    public final void confirm() {
        mResult = true;
        wakeUp();
    }


    JsResult(ResultReceiver receiver) {
        mReceiver = receiver;
    }


    final boolean getResult() {
        return mResult;
    }

    private final void wakeUp() {
        mReceiver.onJsResultComplete(this);
    }
}
