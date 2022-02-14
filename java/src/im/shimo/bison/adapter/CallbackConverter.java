package im.shimo.bison.adapter;

import org.chromium.base.Callback;

import androidx.annotation.RestrictTo;
import im.shimo.bison.ValueCallback;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public final class CallbackConverter {

    public static <T> Callback<T> fromValueCallback(final ValueCallback<T> valueCallback) {
        return valueCallback == null ? null : result -> valueCallback.onReceiveValue(result);
    }

    // Do not instantiate this class
    private CallbackConverter() {}
}
