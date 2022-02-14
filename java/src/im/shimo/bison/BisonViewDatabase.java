package im.shimo.bison;

import im.shimo.bison.internal.BvFormDatabase;

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
        return BvFormDatabase.hasFormData();
    }

    public void clearFormData() {
        BvFormDatabase.clearFormData();
    }

}
