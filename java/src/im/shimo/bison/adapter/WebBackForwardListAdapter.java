package im.shimo.bison.adapter;

import im.shimo.bison.WebBackForwardList;
import im.shimo.bison.WebHistoryItem;

import org.chromium.content_public.browser.NavigationHistory;

import androidx.annotation.RestrictTo;

import java.util.ArrayList;
import java.util.List;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public class WebBackForwardListAdapter extends WebBackForwardList {

    private final List<WebHistoryItemAdapter> mHistroryItemList;
    private final int mCurrentIndex;

    public WebBackForwardListAdapter(NavigationHistory navHistory) {
        mCurrentIndex = navHistory.getCurrentEntryIndex();
        mHistroryItemList = new ArrayList<WebHistoryItemAdapter>(navHistory.getEntryCount());
        for (int i = 0; i < navHistory.getEntryCount(); ++i) {
            mHistroryItemList.add(new WebHistoryItemAdapter(navHistory.getEntryAtIndex(i)));
        }
    }

    /**
     * See {@link android.webkit.WebBackForwardList#getCurrentItem}.
     */
    @Override
    public synchronized WebHistoryItem getCurrentItem() {
        if (getSize() == 0) {
            return null;
        } else {
            return getItemAtIndex(getCurrentIndex());
        }
    }

    /**
     * See {@link android.webkit.WebBackForwardList#getCurrentIndex}.
     */
    @Override
    public synchronized int getCurrentIndex() {
        return mCurrentIndex;
    }

    /**
     * See {@link android.webkit.WebBackForwardList#getItemAtIndex}.
     */
    @Override
    public synchronized WebHistoryItem getItemAtIndex(int index) {
        if (index < 0 || index >= getSize()) {
            return null;
        } else {
            return mHistroryItemList.get(index);
        }
    }

    /**
     * See {@link android.webkit.WebBackForwardList#getSize}.
     */
    @Override
    public synchronized int getSize() {
        return mHistroryItemList.size();
    }

    // Clone constructor.
    private WebBackForwardListAdapter(List<WebHistoryItemAdapter> list, int currentIndex) {
        mHistroryItemList = list;
        mCurrentIndex = currentIndex;
    }

    /**
     * See {@link android.webkit.WebBackForwardList#clone}.
     */
    @Override
    protected synchronized WebBackForwardListAdapter clone() {
        List<WebHistoryItemAdapter> list = new ArrayList<WebHistoryItemAdapter>(getSize());
        for (int i = 0; i < getSize(); ++i) {
            list.add(mHistroryItemList.get(i).clone());
        }
        return new WebBackForwardListAdapter(list, mCurrentIndex);
    }

}
