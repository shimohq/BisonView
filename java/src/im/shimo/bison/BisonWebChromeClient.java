package im.shimo.bison;

/**
 * same as android.webkit.WebChromeClient
 *
 */
public class BisonWebChromeClient {

    /**
     * Tell the host application the current progress of loading a page.
     * @param view The BisonView that initiated the callback.
     * @param newProgress Current page loading progress, represented by
     *                    an integer between 0 and 100.
     */
    public void onProgressChanged(BisonView view, int newProgress) {
    }

    /**
     * Notify the host application of a change in the document title.
     * @param view The BisonView that initiated the callback.
     * @param title A String containing the new title of the document.
     */
    public void onReceivedTitle(BisonView view, String title) {
    }

    public boolean onCreateWindow(BisonView view,boolean isDialog, boolean isUserGesture) {
        return false;
    }

    public void onRequestFocus(BisonView view){}

    public void onCloseWindow(BisonView window) {}

    public boolean onJsAlert(BisonView view, String url, String message,
        JsResult result) {
        return false;
    }

    public boolean onJsConfirm(BisonView view, String url, String message,
        JsResult result) {
        return false;
    }

    public boolean onJsPrompt(BisonView view, String url, String message,
                        String defaultValue, JsPromptResult result) {
        return false;
    }

    public boolean onJsBeforeUnload(BisonView view, String url, String message,
            JsResult result) {
        return false;
    }

    public void onGeolocationPermissionsShowPrompt(String origin,GeolocationPermissions.Callback callback) {}

    public void onGeolocationPermissionsHidePrompt() {}

    public void onPermissionRequest(PermissionRequest request) {
        request.deny();
    }

    public void onPermissionRequestCanceled(PermissionRequest request) {}


}
