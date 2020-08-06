package im.shimo.bison;

import android.graphics.Bitmap;
import android.view.ViewGroup;

import org.chromium.ui.base.ViewAndroidDelegate;
import org.chromium.ui_base.web.CursorType;


public class BisonViewAndroidDelegate extends ViewAndroidDelegate {
 
    public interface OnCursorUpdateHelper {
        void notifyCalled(int type);
    }

    private OnCursorUpdateHelper mOnCursorUpdateHelper;

    public BisonViewAndroidDelegate(ViewGroup containerView) {
        super(containerView);
    }

    public void setOnCursorUpdateHelper(OnCursorUpdateHelper helper) {
        mOnCursorUpdateHelper = helper;
    }

    public OnCursorUpdateHelper getOnCursorUpdateHelper() {
        return mOnCursorUpdateHelper;
    }

    @Override
    public void onCursorChangedToCustom(Bitmap customCursorBitmap, int hotspotX, int hotspotY) {
        super.onCursorChangedToCustom(customCursorBitmap, hotspotX, hotspotY);
        if (mOnCursorUpdateHelper != null) {
            mOnCursorUpdateHelper.notifyCalled(CursorType.CUSTOM);
        }
    }

    @Override
    public void onCursorChanged(int cursorType) {
        super.onCursorChanged(cursorType);
        if (mOnCursorUpdateHelper != null) {
            mOnCursorUpdateHelper.notifyCalled(cursorType);
        }
    }
}
