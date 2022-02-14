package im.shimo.bison;

import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Message;
import android.view.View;

import im.shimo.bison.internal.BvContentsClient;

import androidx.annotation.Nullable;

public class BisonWebChromeClient {

     /**
     * Tell the host application the current progress of loading a page.
     *
     * @param view The BisonView that initiated the callback.
     * @param newProgress Current page loading progress, represented by an integer between 0 and
     *        100.
     */
    public void onProgressChanged(BisonView view, int newProgress) {}

    /**
     * Notify the host application of a change in the document title.
     *
     * @param view The BisonView that initiated the callback.
     * @param title A String containing the new title of the document.
     */
    public void onReceivedTitle(BisonView view, String title) {}

    public void onReceivedIcon(BisonView view, Bitmap icon) {}

    public void onReceivedTouchIconUrl(BisonView view, String url, boolean precomposed) {}

    public interface CustomViewCallback {
        /**
         * Invoked when the host application dismisses the custom view.
         */
        public void onCustomViewHidden();
    }

    public void onShowCustomView(View view, CustomViewCallback callback) {};

    public void onHideCustomView() {}



    public boolean onCreateWindow(BisonView view, boolean isDialog, boolean isUserGesture,
            Message resultMsg) {
        return false;
    }

    public void onRequestFocus(BisonView view) {}

    public void onCloseWindow(BisonView window) {}

    public boolean onJsAlert(BisonView view, String url, String message, JsResult result) {
        return false;
    }

    public boolean onJsConfirm(BisonView view, String url, String message, JsResult result) {
        return false;
    }

    public boolean onJsPrompt(BisonView view, String url, String message, String defaultValue,
            JsPromptResult result) {
        return false;
    }

    public boolean onJsBeforeUnload(BisonView view, String url, String message, JsResult result) {
        return false;
    }

    public void onGeolocationPermissionsShowPrompt(String origin,
            GeolocationPermissions.Callback callback) {}

    public void onGeolocationPermissionsHidePrompt() {}

    public void onPermissionRequest(PermissionRequest request) {
        request.deny();
    }



    public void onPermissionRequestCanceled(PermissionRequest request) {}

    /**
     * BvWebContentsDelegate#addMessageToConsole
     * @param consoleMessage
     * @return
     */
    public boolean onConsoleMessage(ConsoleMessage consoleMessage) {
        return false;
    }

    @Nullable
    public Bitmap getDefaultVideoPoster() {
        return null;
    }

    @Nullable
    public View getVideoLoadingProgressView() {
        return null;
    }

    /**
     * Obtains a list of all visited history items, used for link coloring
     */
    public void getVisitedHistory(ValueCallback<String[]> callback) {}

    public boolean onShowFileChooser(BisonView view, ValueCallback<Uri[]> filePathCallback,
            FileChooserParams fileChooserParams) {
        return false;
    }



    public static abstract class FileChooserParams {
        /** Open single file. Requires that the file exists before allowing the user to pick it. */
        public static final int MODE_OPEN = 0;
        /** Like Open but allows multiple files to be selected. */
        public static final int MODE_OPEN_MULTIPLE = 1;
        /**
         * Like Open but allows a folder to be selected. The implementation should enumerate all
         * files selected by this operation. This feature is not supported at the moment.
         *
         * @hide
         */
        public static final int MODE_OPEN_FOLDER = 2;
        /** Allows picking a nonexistent file and saving it. */
        public static final int MODE_SAVE = 3;

        /**
         * Parse the result returned by the file picker activity. This method should be used with
         * {@link #createIntent}. Refer to {@link #createIntent} for how to use it.
         *
         * @param resultCode the integer result code returned by the file picker activity.
         * @param data the intent returned by the file picker activity.
         * @return the Uris of selected file(s) or {@code null} if the resultCode indicates activity
         *         canceled or any other error.
         */
        @Nullable
        public static Uri[] parseResult(int resultCode, Intent data) {
            return BvContentsClient.parseFileChooserResult(resultCode, data);
        }

        /**
         * Returns file chooser mode.
         */
        public abstract int getMode();

        /**
         * Returns an array of acceptable MIME types. The returned MIME type could be partial such
         * as audio/*. The array will be empty if no acceptable types are specified.
         */
        public abstract String[] getAcceptTypes();

        /**
         * Returns preference for a live media captured value (e.g. Camera, Microphone). True
         * indicates capture is enabled, {@code false} disabled.
         *
         * Use <code>getAcceptTypes</code> to determine suitable capture devices.
         */
        public abstract boolean isCaptureEnabled();

        /**
         * Returns the title to use for this file selector. If {@code null} a default title should
         * be used.
         */
        @Nullable
        public abstract CharSequence getTitle();

        /**
         * The file name of a default selection if specified, or {@code null}.
         */
        @Nullable
        public abstract String getFilenameHint();

        /**
         * Creates an intent that would start a file picker for file selection. The Intent supports
         * choosing files from simple file sources available on the device. Some advanced sources
         * (for example, live media capture) may not be supported and applications wishing to
         * support these sources or more advanced file operations should build their own Intent.
         *
         * <p>
         * How to use:
         * <ol>
         * <li>Build an intent using {@link #createIntent}</li>
         * <li>Fire the intent using {@link android.app.Activity#startActivityForResult}.</li>
         * <li>Check for ActivityNotFoundException and take a user friendly action if thrown.</li>
         * <li>Listen the result using {@link android.app.Activity#onActivityResult}</li>
         * <li>Parse the result using {@link #parseResult} only if media capture was not
         * requested.</li>
         * <li>Send the result using filePathCallback of
         * {@link WebChromeClient#onShowFileChooser}</li>
         * </ol>
         *
         * @return an Intent that supports basic file chooser sources.
         */
        public abstract Intent createIntent();
    }

}
