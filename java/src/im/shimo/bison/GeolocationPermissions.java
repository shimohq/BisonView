package im.shimo.bison;

import java.util.Set;

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

    // public static GeolocationPermissions getInstance() {
    //   return WebViewFactory.getProvider().getGeolocationPermissions();
    // }

    private GeolocationPermissions(){}

    public void getOrigins(ValueCallback<Set<String> > callback) {
        // Must be a no-op for backward compatibility: see the hidden constructor for reason.
    }

    public void getAllowed(String origin, ValueCallback<Boolean> callback) {
        // Must be a no-op for backward compatibility: see the hidden constructor for reason.
    }

    public void clear(String origin) {
        // Must be a no-op for backward compatibility: see the hidden constructor for reason.
    }

    public void allow(String origin) {
        // Must be a no-op for backward compatibility: see the hidden constructor for reason.
    }




}