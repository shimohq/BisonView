package im.shimo.bison;

import android.annotation.SuppressLint;
import android.content.Context;
import android.net.Uri;
import android.provider.MediaStore;

import org.chromium.base.Callback;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.ContentUriUtils;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.task.AsyncTask;
import org.chromium.components.embedder_support.delegate.WebContentsDelegateAndroid;

@JNINamespace("bison")
public class BisonWebContentsDelegate extends WebContentsDelegateAndroid {

    protected static final boolean TRACE = false;

    private BisonContentsClient mContentsClient;
    private BisonContents mBisonContents;
    private Context mContext;

    public BisonWebContentsDelegate(Context context, BisonContents bisonContents , BisonContentsClient bisonContentsClient) {
        mBisonContents = bisonContents;
        mContentsClient = bisonContentsClient;
    }

    // @Override
    // public void onLoadProgressChanged(int progress) {
    //     mContentsClient.getCallbackHelper().postOnProgressChanged(progress);
    // }


    @CalledByNative
    public void runFileChooser(int processId, int renderId, int modeFlags,
            String acceptTypes, String title, String defaultFilename,  boolean capture){
        BisonContentsClient.FileChooserParamsImpl params = new BisonContentsClient.FileChooserParamsImpl(
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
                    BisonWebContentsDelegateJni.get().filesSelectedInChooser(
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
        // jiang unimpl
        /* return mContentsClient.onCreateWindow(isDialog, isUserGesture); */
        return false;
    }

    @Override
    @CalledByNative
    public void closeContents(){
        // jiang unimpl
        //mContentsClient.onCloseWindow();
    }

    @Override
    @CalledByNative
    public void activateContents(){
        mContentsClient.onRequestFocus();
    }

    @Override
    @CalledByNative
    public void navigationStateChanged(int flags){
        // jiang unimpl
    }

    // Not an override, because WebContentsDelegateAndroid maps this call
    // into onLoad{Started|Stopped}.
    @CalledByNative
    public void loadingStateChanged(){
         mContentsClient.updateTitle(mBisonContents.getTitle(), false);
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
            BisonWebContentsDelegateJni.get().filesSelectedInChooser(
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