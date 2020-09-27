package im.shimo.bison;

import org.chromium.base.Callback;

final class CallbackConverter {
    public static <T> Callback<T> fromValueCallback(final ValueCallback<T> valueCallback) {
        return valueCallback == null ? null : result -> valueCallback.onReceiveValue(result);
    }

    // Do not instantiate this class
    private CallbackConverter() {}
}