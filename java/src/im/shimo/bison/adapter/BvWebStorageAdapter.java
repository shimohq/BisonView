package im.shimo.bison.adapter;

import im.shimo.bison.BisonViewWebStorage;
import im.shimo.bison.ValueCallback;
import im.shimo.bison.internal.BvQuotaManagerBridge;

import org.chromium.base.Callback;
import org.chromium.base.ThreadUtils;

import java.util.HashMap;
import java.util.Map;

import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public class BvWebStorageAdapter extends BisonViewWebStorage {
    private final BvQuotaManagerBridge mQuotaManagerBridge;

    public BvWebStorageAdapter(BvQuotaManagerBridge quotaManagerBridge) {
        mQuotaManagerBridge = quotaManagerBridge;
    }

    @Override
    public void getOrigins(final ValueCallback<Map<String, Origin>> callback) {
        final Callback<BvQuotaManagerBridge.Origins> bisonOriginsCallback = new Callback<BvQuotaManagerBridge.Origins>() {
            @Override
            public void onResult(BvQuotaManagerBridge.Origins origins) {
                Map<String, Origin> originsMap = new HashMap<String, Origin>();
                for (int i = 0; i < origins.mOrigins.length; ++i) {
                    Origin origin = new Origin(origins.mOrigins[i], origins.mQuotas[i], origins.mUsages[i]) {
                    };
                    originsMap.put(origins.mOrigins[i], origin);
                }
                callback.onReceiveValue(originsMap);
            }
        };
        // if (checkNeedsPost()) {
        // mFactory.addTask(new Runnable() {
        // @Override
        // public void run() {
        // mQuotaManagerBridge.getOrigins(bisonOriginsCallback);
        // }

        // });
        // return;
        // }
        mQuotaManagerBridge.getOrigins(bisonOriginsCallback);
    }

    @Override
    public void getUsageForOrigin(final String origin, final ValueCallback<Long> callback) {
        // if (checkNeedsPost()) {
        // mFactory.addTask(new Runnable() {
        // @Override
        // public void run() {
        // mQuotaManagerBridge.getUsageForOrigin(
        // origin, CallbackConverter.fromValueCallback(callback));
        // }

        // });
        // return;
        // }
        mQuotaManagerBridge.getUsageForOrigin(origin, CallbackConverter.fromValueCallback(callback));
    }

    @Override
    public void getQuotaForOrigin(final String origin, final ValueCallback<Long> callback) {
        // if (checkNeedsPost()) {
        // mFactory.addTask(new Runnable() {
        // @Override
        // public void run() {
        // mQuotaManagerBridge.getQuotaForOrigin(
        // origin, CallbackConverter.fromValueCallback(callback));
        // }

        // });
        // return;
        // }
        mQuotaManagerBridge.getQuotaForOrigin(origin, CallbackConverter.fromValueCallback(callback));
    }

    @Override
    public void setQuotaForOrigin(String origin, long quota) {
        // Intentional no-op for deprecated method.
    }

    @Override
    public void deleteOrigin(final String origin) {
        // if (checkNeedsPost()) {
        // mFactory.addTask(new Runnable() {
        // @Override
        // public void run() {
        // mQuotaManagerBridge.deleteOrigin(origin);
        // }

        // });
        // return;
        // }
        mQuotaManagerBridge.deleteOrigin(origin);
    }

    @Override
    public void deleteAllData() {
        // if (checkNeedsPost()) {
        // mFactory.addTask(new Runnable() {
        // @Override
        // public void run() {
        // mQuotaManagerBridge.deleteAllData();
        // }

        // });
        // return;
        // }
        mQuotaManagerBridge.deleteAllData();
    }

    private static boolean checkNeedsPost() {
        return !ThreadUtils.runningOnUiThread();
    }

}
