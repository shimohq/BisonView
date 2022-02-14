package im.shimo.bison.adapter;

import java.util.Set;

import androidx.annotation.RestrictTo;
import im.shimo.bison.GeolocationPermissions;
import im.shimo.bison.ValueCallback;
import im.shimo.bison.internal.BvGeolocationPermissions;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public class GeolocationPermissionsAdapter extends GeolocationPermissions {

    private final BvGeolocationPermissions mGeolocationPermissions;

    public GeolocationPermissionsAdapter(BvGeolocationPermissions geolocationPermissions){
        mGeolocationPermissions = geolocationPermissions;
    }

    @Override
    public void allow(final String origin) {
        mGeolocationPermissions.allow(origin);
    }

    @Override
    public void clear(final String origin) {
        mGeolocationPermissions.clear(origin);
    }

    @Override
    public void clearAll() {
        mGeolocationPermissions.clearAll();
    }

    @Override
    public void getAllowed(final String origin, final ValueCallback<Boolean> callback) {
        mGeolocationPermissions.getAllowed(
                origin, CallbackConverter.fromValueCallback(callback));
    }

    @Override
    public void getOrigins(final ValueCallback<Set<String>> callback) {
        mGeolocationPermissions.getOrigins(CallbackConverter.fromValueCallback(callback));
    }

}
