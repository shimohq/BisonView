package im.shimo.bison;

import android.graphics.Bitmap;

import androidx.annotation.Nullable;

public abstract class WebHistoryItem implements Cloneable {
    /**
     * Return an identifier for this history item. If an item is a copy of
     * another item, the identifiers will be the same even if they are not the
     * same object.
     * @return The id for this item.
     * @deprecated This method is now obsolete.
     * @hide Since API level {@link android.os.Build.VERSION_CODES#JELLY_BEAN_MR1}
     */
    @SuppressWarnings("HiddenAbstractMethod")
    public abstract int getId();

    /**
     * Return the url of this history item. The url is the base url of this
     * history item. See getTargetUrl() for the url that is the actual target of
     * this history item.
     * @return The base url of this history item.
     */
    public abstract String getUrl();

    /**
     * Return the original url of this history item. This was the requested
     * url, the final url may be different as there might have been
     * redirects while loading the site.
     * @return The original url of this history item.
     */
    public abstract String getOriginalUrl();

    /**
     * Return the document title of this history item.
     * @return The document title of this history item.
     */
    public abstract String getTitle();

    /**
     * Return the favicon of this history item or {@code null} if no favicon was found.
     * @return A Bitmap containing the favicon for this history item or {@code null}.
     */
    @Nullable
    public abstract Bitmap getFavicon();

    /**
     * Clone the history item for use by clients of WebView. On Android 4.4 and later
     * there is no need to use this, as the object is already a read-only copy of the
     * internal state.
     */
    @Override
    protected abstract WebHistoryItem clone();
}
