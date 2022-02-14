package im.shimo.bison;

import static androidx.annotation.RestrictTo.Scope.LIBRARY_GROUP;

import android.os.Handler;

import androidx.annotation.RestrictTo;

public class HttpAuthHandler extends Handler {

    @RestrictTo(LIBRARY_GROUP)
    public HttpAuthHandler() {
    }


    public boolean useHttpAuthUsernamePassword() {
        return false;
    }

    public void cancel() {
    }

    public void proceed(String username, String password) {
    }
}
