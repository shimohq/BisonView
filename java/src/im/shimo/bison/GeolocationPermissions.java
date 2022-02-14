package im.shimo.bison;

import java.util.Set;

import androidx.annotation.RestrictTo;

public class GeolocationPermissions {

    public interface Callback {
        /**
         * Sets the Geolocation permission state for the supplied origin.
         *
         * @param origin the origin for which permissions are set
         * @param allow whether or not the origin should be allowed to use the
         *              Geolocation API
         * @param retain whether the permission should be retained beyond the
         *               lifetime of a page currently being displayed by a
         *               WebView
         */
        void invoke(String origin, boolean allow, boolean retain);
    };

    @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
    public GeolocationPermissions(){}

    public void getOrigins(ValueCallback<Set<String> > callback) {
    }

    public void getAllowed(String origin, ValueCallback<Boolean> callback) {
    }

    public void clear(String origin) {
    }

    public void allow(String origin) {
    }

    public void clearAll() {
    }
}
