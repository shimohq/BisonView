package im.shimo.bison.internal;

import android.annotation.SuppressLint;
import android.content.Context;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.provider.MediaStore;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.webkit.URLUtil;
import android.widget.FrameLayout;

import org.chromium.base.Callback;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.ContentUriUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.task.AsyncTask;
import org.chromium.components.embedder_support.delegate.WebContentsDelegateAndroid;
import org.chromium.content_public.browser.InvalidateTypes;
import org.chromium.content_public.common.ContentUrlConstants;
import org.chromium.content_public.common.ResourceRequestBody;

/**
 * BisonView-specific WebContentsDelegate.
 * This file is the Java version of the native class of the same name.
 * This class provides auxiliary functions for routing certain callbacks from the content layer
 * to specific listener interfaces.
 */
@JNINamespace("bison")
public class BvWebContentsDelegate extends WebContentsDelegateAndroid {
    private static final String TAG = "BvWebContentsDelegate";
    protected static final boolean TRACE = false;

    private final BvContentsClient mContentsClient;
    private final BvContents mBvContents;
    private final BvSettings mBvSettings;
    private final Context mContext;
    private View mContainerView;
    private FrameLayout mCustomView;


    public BvWebContentsDelegate(Context context, BvContents bvContents ,
            BvContentsClient contentsClient,BvSettings settings, View containerView) {
        mBvContents = bvContents;
        mContentsClient = contentsClient;
        mBvSettings = settings;
        mContext = context;
        setContainerView(containerView);
    }

    public void setContainerView(View containerView) {
        mContainerView = containerView;
        mContainerView.setClickable(true);
    }

    @Override
    public void handleKeyboardEvent(KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            int direction;
            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    direction = View.FOCUS_DOWN;
                    break;
                case KeyEvent.KEYCODE_DPAD_UP:
                    direction = View.FOCUS_UP;
                    break;
                case KeyEvent.KEYCODE_DPAD_LEFT:
                    direction = View.FOCUS_LEFT;
                    break;
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    direction = View.FOCUS_RIGHT;
                    break;
                default:
                    direction = 0;
                    break;
            }
            if (direction != 0 && tryToMoveFocus(direction)) return;
        }
        handleMediaKey(event);
        mContentsClient.onUnhandledKeyEvent(event);
    }

    private void handleMediaKey(KeyEvent e) {
        switch (e.getKeyCode()) {
            case KeyEvent.KEYCODE_MUTE:
            case KeyEvent.KEYCODE_HEADSETHOOK:
            case KeyEvent.KEYCODE_MEDIA_PLAY:
            case KeyEvent.KEYCODE_MEDIA_PAUSE:
            case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
            case KeyEvent.KEYCODE_MEDIA_STOP:
            case KeyEvent.KEYCODE_MEDIA_NEXT:
            case KeyEvent.KEYCODE_MEDIA_PREVIOUS:
            case KeyEvent.KEYCODE_MEDIA_REWIND:
            case KeyEvent.KEYCODE_MEDIA_RECORD:
            case KeyEvent.KEYCODE_MEDIA_FAST_FORWARD:
            case KeyEvent.KEYCODE_MEDIA_CLOSE:
            case KeyEvent.KEYCODE_MEDIA_EJECT:
            case KeyEvent.KEYCODE_MEDIA_AUDIO_TRACK:
                AudioManager am = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
                am.dispatchMediaKeyEvent(e);
                break;
            default:
                break;
        }
    }
    // @Override
    // public void onLoadProgressChanged(int progress) {
    //     mContentsClient.getCallbackHelper().postOnProgressChanged(progress);
    // }

    @Override
    public boolean takeFocus(boolean reverse) {
        int direction =
                (reverse == (mContainerView.getLayoutDirection() == View.LAYOUT_DIRECTION_RTL))
                ? View.FOCUS_RIGHT : View.FOCUS_LEFT;
        if (tryToMoveFocus(direction)) return true;
        direction = reverse ? View.FOCUS_BACKWARD : View.FOCUS_FORWARD;
        return tryToMoveFocus(direction);
    }

    private boolean tryToMoveFocus(int direction) {
        View focus = mContainerView.focusSearch(direction);
        return focus != null && focus != mContainerView && focus.requestFocus();
    }

    @Override
    public boolean addMessageToConsole(int level, String message, int lineNumber,
            String sourceId) {
        @BvConsoleMessage.MessageLevel
        int messageLevel = BvConsoleMessage.MESSAGE_LEVEL_DEBUG;
        switch(level) {
            case LOG_LEVEL_TIP:
                messageLevel = BvConsoleMessage.MESSAGE_LEVEL_TIP;
                break;
            case LOG_LEVEL_LOG:
                messageLevel = BvConsoleMessage.MESSAGE_LEVEL_LOG;
                break;
            case LOG_LEVEL_WARNING:
                messageLevel = BvConsoleMessage.MESSAGE_LEVEL_WARNING;
                break;
            case LOG_LEVEL_ERROR:
                messageLevel = BvConsoleMessage.MESSAGE_LEVEL_ERROR;
                break;
            default:
                Log.w(TAG, "Unknown message level, defaulting to DEBUG");
                break;
        }
        boolean result = mContentsClient.onConsoleMessage(
                new BvConsoleMessage(message, sourceId, lineNumber, messageLevel));
        return result;
    }

    @Override
    public void onUpdateUrl(String url) {
        // TODO: implement
    }

    @Override
    public void openNewTab(String url, String extraHeaders, ResourceRequestBody postData,
            int disposition, boolean isRendererInitiated) {
        // This is only called in chrome layers.
        assert false;
    }

    @Override
    @CalledByNative
    public void closeContents(){
        mContentsClient.onCloseWindow();
    }

    @Override
    @SuppressLint("HandlerLeak")
    public void showRepostFormWarningDialog() {
        final int msgContinuePendingReload = 1;
        final int msgCancelPendingReload = 2;

        final Handler handler = new Handler(ThreadUtils.getUiThreadLooper()) {
            @Override
            public void handleMessage(Message msg) {
                if (mBvContents.getNavigationController() == null) return;

                switch(msg.what) {
                    case msgContinuePendingReload: {
                        mBvContents.getNavigationController().continuePendingReload();
                        break;
                    }
                    case msgCancelPendingReload: {
                        mBvContents.getNavigationController().cancelPendingReload();
                        break;
                    }
                    default:
                        throw new IllegalStateException(
                                "WebContentsDelegateAdapter: unhandled message " + msg.what);
                }
            }
        };

        Message resend = handler.obtainMessage(msgContinuePendingReload);
        Message dontResend = handler.obtainMessage(msgCancelPendingReload);
        mContentsClient.getCallbackHelper().postOnFormResubmission(dontResend, resend);
    }

    @CalledByNative
    public void runFileChooser(int processId, int renderId, int modeFlags,
                String acceptTypes, String title, String defaultFilename,  boolean capture){
        BvContentsClient.FileChooserParamsImpl params = new BvContentsClient.FileChooserParamsImpl(
                modeFlags, acceptTypes, title, defaultFilename, capture);

        mContentsClient.showFileChooser(new Callback<String[]>() {
            boolean mCompleted;
            @Override
            public void onResult(String[] results) {
                if (mCompleted) {
                    throw new IllegalStateException("Duplicate showFileChooser result");
                }
                mCompleted = true;
                if (results == null) {
                    BvWebContentsDelegateJni.get().filesSelectedInChooser(
                            processId, renderId, modeFlags, null, null);
                    return;
                }
                GetDisplayNameTask task =
                        new GetDisplayNameTask(mContext, processId, renderId, modeFlags, results);
                task.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
            }
        }, params);
    }

    @CalledByNative
    public boolean addNewContents(boolean isDialog, boolean isUserGesture){
        return mContentsClient.onCreateWindow(isDialog, isUserGesture);
    }

    @Override
    @CalledByNative
    public void activateContents(){
        mContentsClient.onRequestFocus();
    }

    @Override
    @CalledByNative
    public void navigationStateChanged(int flags){
        if ((flags & InvalidateTypes.URL) != 0
               /* && mBvContents.isPopupWindow() */
                && mBvContents.hasAccessedInitialDocument()) {
            // Hint the client to show the last committed url, as it may be unsafe to show
            // the pending entry.
            String url = mBvContents.getLastCommittedUrl();
            url = TextUtils.isEmpty(url) ? ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL : url;
            mContentsClient.getCallbackHelper().postSynthesizedPageLoadingForUrlBarUpdate(url);
        }
    }

    // jiang full screen view
    @Override
    public void enterFullscreenModeForTab(boolean prefersNavigationBar) {
        enterFullscreen();
    }

    @Override
    public void exitFullscreenModeForTab() {
        exitFullscreen();
    }

    @CalledByNative
    public void loadingStateChanged(){
         mContentsClient.updateTitle(mBvContents.getTitle(), false);
    }

    private void enterFullscreen() {
        // jiang
        // if (mBvContents.isFullScreen()) {
        //     return;
        // }
        // View fullscreenView = mBvContents.enterFullScreen();
        // if (fullscreenView == null) {
        //     return;
        // }
        // BvContentsClient.CustomViewCallback cb = () -> {
        //     if (mCustomView != null) {
        //         mBvContents.requestExitFullscreen();
        //     }
        // };
        // mCustomView = new FrameLayout(mContext);
        // mCustomView.addView(fullscreenView);
        // mContentsClient.onShowCustomView(mCustomView, cb);
    }

    /**
     * Called to show the web contents in embedded mode.
     */
    private void exitFullscreen() {
        // jiang
        // if (mCustomView != null) {
        //     mCustomView = null;
        //     mBvContents.exitFullScreen();
        //     mContentsClient.onHideCustomView();
        // }
    }

    @Override
    public boolean shouldBlockMediaRequest(String url) {
        return mBvSettings != null
                ? mBvSettings.getBlockNetworkLoads() && URLUtil.isNetworkUrl(url) : true;
    }

    private static class GetDisplayNameTask extends AsyncTask<String[]> {
        final int mProcessId;
        final int mRenderId;
        final int mModeFlags;
        final String[] mFilePaths;

        // The task doesn't run long, so we don't gain anything from a weak ref.
        @SuppressLint("StaticFieldLeak")
        final Context mContext;

        public GetDisplayNameTask(
                Context context, int processId, int renderId, int modeFlags, String[] filePaths) {
            mProcessId = processId;
            mRenderId = renderId;
            mModeFlags = modeFlags;
            mFilePaths = filePaths;
            mContext = context;
        }

        @Override
        protected String[] doInBackground() {
            String[] displayNames = new String[mFilePaths.length];
            for (int i = 0; i < mFilePaths.length; i++) {
                displayNames[i] = resolveFileName(mFilePaths[i]);
            }
            return displayNames;
        }

        @Override
        protected void onPostExecute(String[] result) {
            BvWebContentsDelegateJni.get().filesSelectedInChooser(
                    mProcessId, mRenderId, mModeFlags, mFilePaths, result);
        }

        /**
         * @return the display name of a path if it is a content URI and is present in the database
         * or an empty string otherwise.
         */
        private String resolveFileName(String filePath) {
            if (filePath == null) return "";
            Uri uri = Uri.parse(filePath);
            return ContentUriUtils.getDisplayName(
                    uri, mContext, MediaStore.MediaColumns.DISPLAY_NAME);
        }
    }

    @NativeMethods
    interface Natives {
        // Call in response to a prior runFileChooser call.
        void filesSelectedInChooser(int processId, int renderId, int modeFlags, String[] filePath,
                String[] displayName);
    }


}
