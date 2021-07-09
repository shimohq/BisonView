package im.shimo.bison;

import org.chromium.base.Callback;
import org.chromium.base.ThreadUtils;

import java.util.HashMap;
import java.util.Map;


public class BisonWebStorage {
    
    private final BisonQuotaManagerBridge mQuotaManagerBridge;

    BisonWebStorage(BisonQuotaManagerBridge quotaManagerBridge){
        mQuotaManagerBridge = quotaManagerBridge;
    }

    public final void getOrigins(final ValueCallback<Map> callback) {
        final Callback<BisonQuotaManagerBridge.Origins> bisonOriginsCallback =
                new Callback<BisonQuotaManagerBridge.Origins>() {
                    @Override
                    public void onResult(BisonQuotaManagerBridge.Origins origins) {
                        Map<String, Origin> originsMap = new HashMap<String, Origin>();
                        for (int i = 0; i < origins.mOrigins.length; ++i) {
                            Origin origin = new Origin(
                                    origins.mOrigins[i], origins.mQuotas[i], origins.mUsages[i]) {
                            };
                            originsMap.put(origins.mOrigins[i], origin);
                        }
                        callback.onReceiveValue(originsMap);
                    }
                };
        // if (checkNeedsPost()) {
        //     mFactory.addTask(new Runnable() {
        //         @Override
        //         public void run() {
        //             mQuotaManagerBridge.getOrigins(bisonOriginsCallback);
        //         }

        //     });
        //     return;
        // }
        mQuotaManagerBridge.getOrigins(bisonOriginsCallback);
    }

    public final void getUsageForOrigin(final String origin, final ValueCallback<Long> callback) {
        // if (checkNeedsPost()) {
        //     mFactory.addTask(new Runnable() {
        //         @Override
        //         public void run() {
        //             mQuotaManagerBridge.getUsageForOrigin(
        //                     origin, CallbackConverter.fromValueCallback(callback));
        //         }

        //     });
        //     return;
        // }
        mQuotaManagerBridge.getUsageForOrigin(
                origin, CallbackConverter.fromValueCallback(callback));
    }

    public final void getQuotaForOrigin(final String origin, final ValueCallback<Long> callback) {
        // if (checkNeedsPost()) {
        //     mFactory.addTask(new Runnable() {
        //         @Override
        //         public void run() {
        //             mQuotaManagerBridge.getQuotaForOrigin(
        //                     origin, CallbackConverter.fromValueCallback(callback));
        //         }

        //     });
        //     return;
        // }
        mQuotaManagerBridge.getQuotaForOrigin(
                origin, CallbackConverter.fromValueCallback(callback));
    }

    
    public final void setQuotaForOrigin(String origin, long quota) {
        // Intentional no-op for deprecated method.
    }

    
    public final void deleteOrigin(final String origin) {
        // if (checkNeedsPost()) {
        //     mFactory.addTask(new Runnable() {
        //         @Override
        //         public void run() {
        //             mQuotaManagerBridge.deleteOrigin(origin);
        //         }

        //     });
        //     return;
        // }
        mQuotaManagerBridge.deleteOrigin(origin);
    }

    public final void deleteAllData() {
        // if (checkNeedsPost()) {
        //     mFactory.addTask(new Runnable() {
        //         @Override
        //         public void run() {
        //             mQuotaManagerBridge.deleteAllData();
        //         }

        //     });
        //     return;
        // }
        mQuotaManagerBridge.deleteAllData();
    }


    private static boolean checkNeedsPost() {
        return !ThreadUtils.runningOnUiThread();
    }

    static class Origin {

        private String mOrigin = null;
        private long mQuota = 0;
        private long mUsage = 0;

        protected Origin(String origin, long quota, long usage) {
            mOrigin = origin;
            mQuota = quota;
            mUsage = usage;
        }

        public String getOrigin() {
            return mOrigin;
        }

        public long getQuota() {
            return mQuota;
        }

        public long getUsage() {
            return mUsage;
        }

    }

}
