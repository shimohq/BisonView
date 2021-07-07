package im.shimo.bison;

import android.os.Build;

public class BisonViewDatabase {
    private final HttpAuthDatabase mHttpAuthDatabase;

    public BisonViewDatabase(HttpAuthDatabase httpAuthDatabase) {
        mHttpAuthDatabase = httpAuthDatabase;
    }

    public boolean hasHttpAuthUsernamePassword() {
        // jiang post task
        return mHttpAuthDatabase.hasHttpAuthUsernamePassword();
    }

    public void clearHttpAuthUsernamePassword() {

        mHttpAuthDatabase.clearHttpAuthUsernamePassword();
    }

    public void setHttpAuthUsernamePassword(final String host, final String realm, final String username,
            final String password) {

        mHttpAuthDatabase.setHttpAuthUsernamePassword(host, realm, username, password);
    }

    public String[] getHttpAuthUsernamePassword(final String host, final String realm) {

        return mHttpAuthDatabase.getHttpAuthUsernamePassword(host, realm);
    }

    public boolean hasFormData() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
            return false;

        return BisonFormDatabase.hasFormData();
    }

    public void clearFormData() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
            return;

        BisonFormDatabase.clearFormData();
    }}
