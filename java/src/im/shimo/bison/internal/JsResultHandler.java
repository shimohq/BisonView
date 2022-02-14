package im.shimo.bison.internal;

import org.chromium.base.task.PostTask;
import org.chromium.content_public.browser.UiThreadTaskTraits;

import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
class JsResultHandler implements JsResultReceiver, JsPromptResultReceiver {
    private BvContentsClientBridge mBridge;
    private final int mId;

    JsResultHandler(BvContentsClientBridge bridge, int id) {
        mBridge = bridge;
        mId = id;
    }

    @Override
    public void confirm() {
        confirm(null);
    }

    @Override
    public void confirm(final String promptResult) {
        PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> {
            if (mBridge != null) mBridge.confirmJsResult(mId, promptResult);
            mBridge = null;
        });
    }

    @Override
    public void cancel() {
        PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> {
            if (mBridge != null) mBridge.cancelJsResult(mId);
            mBridge = null;
        });
    }
}
