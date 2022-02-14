package im.shimo.bison.adapter;

import android.content.Context;

import im.shimo.bison.BisonInitializer;
import im.shimo.bison.BisonViewDatabase;
import im.shimo.bison.BisonViewWebStorage;
import im.shimo.bison.CookieManager;
import im.shimo.bison.HttpAuthDatabase;
import im.shimo.bison.internal.BvCookieManager;

import org.chromium.base.ContextUtils;

import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public class BisonViewWebObjectProvider {
    private static final String HTTP_AUTH_DATABASE_FILE = "http_auth.db";
    private static volatile BisonViewWebObjectProvider INSTANCE;

    private CookieManager mCookieManager;
    private BisonViewDatabase mDatabase;
    private BisonViewWebStorage mWebStorage;

    public static BisonViewWebObjectProvider getInstance() {
        if (INSTANCE == null) {
            synchronized (BisonViewWebObjectProvider.class) {
                if (INSTANCE == null) {
                    INSTANCE = new BisonViewWebObjectProvider();
                }
            }
        }
        return INSTANCE;
    }

    private BisonViewWebObjectProvider() {
        BisonInitializer.getInstance().ensureStarted();
    }

    public CookieManager getCookieManager() {
        if (mCookieManager == null) {
            mCookieManager = new CookieManagerAdapter(new BvCookieManager());
        }
        return mCookieManager;
    }

    public BisonViewWebStorage getWebStorage() {
        mWebStorage = new BvWebStorageAdapter(
                BisonInitializer.getInstance().getBrowserContext().getQuotaManagerBridge());
        return mWebStorage;
    }

    public BisonViewDatabase getBisonViewDatabase() {
        if (mDatabase == null) {
            mDatabase = new BisonViewDatabase(HttpAuthDatabase.newInstance(getContext(), HTTP_AUTH_DATABASE_FILE));
        }
        return mDatabase;
    }

    private Context getContext() {
        return ContextUtils.getApplicationContext();
    }

}
