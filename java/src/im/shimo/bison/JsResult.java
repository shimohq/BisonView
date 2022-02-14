package im.shimo.bison;

import androidx.annotation.RestrictTo;

public class JsResult {

    @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
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

    @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
    JsResult(ResultReceiver receiver) {
        mReceiver = receiver;
    }

    @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
    public
    final boolean getResult() {
        return mResult;
    }

    private final void wakeUp() {
        mReceiver.onJsResultComplete(this);
    }

}
