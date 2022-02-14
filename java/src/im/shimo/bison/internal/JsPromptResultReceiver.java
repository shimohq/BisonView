package im.shimo.bison.internal;

import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public interface JsPromptResultReceiver {
    void confirm(String result);
    void cancel();
}
