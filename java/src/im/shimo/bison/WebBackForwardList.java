package im.shimo.bison;

import androidx.annotation.Nullable;

import java.io.Serializable;

public abstract class WebBackForwardList implements Cloneable, Serializable {
    /**
     * Return the current history item. This method returns {@code null} if the list is
     * empty.
     *
     * @return The current history item.
     */
    @Nullable
    public abstract WebHistoryItem getCurrentItem();

    /**
     * Get the index of the current history item. This index can be used to
     * directly index into the array list.
     *
     * @return The current index from 0...n or -1 if the list is empty.
     */
    public abstract int getCurrentIndex();

    /**
     * Get the history item at the given index. The index range is from 0...n
     * where 0 is the first item and n is the last item.
     *
     * @param index The index to retrieve.
     */
    public abstract WebHistoryItem getItemAtIndex(int index);

    /**
     * Get the total size of the back/forward list.
     *
     * @return The size of the list.
     */
    public abstract int getSize();

    /**
     * Clone the entire object to be used in the UI thread by clients of
     * WebView. This creates a copy that should never be modified by any of the
     * webkit package classes. On Android 4.4 and later there is no need to use
     * this, as the object is already a read-only copy of the internal state.
     */
    @Override
    protected abstract WebBackForwardList clone();
}
