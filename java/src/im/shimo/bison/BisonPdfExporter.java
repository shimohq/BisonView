package im.shimo.bison;

import android.annotation.SuppressLint;
import android.os.CancellationSignal;
import android.os.ParcelFileDescriptor;
import android.print.PrintAttributes;
import android.util.Log;
import android.view.ViewGroup;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

/**
 * Export the android webview as a PDF.
 * @TODO(sgurun) explain the ownership of this class and its native counterpart
 */
@SuppressLint("NewApi")  // Printing requires API level 19.
@JNINamespace("bison")
public class BisonPdfExporter {

    private static final String TAG = "BisonPdfExporter";
    private long mNativeBisonPdfExporter;
    // TODO(sgurun) result callback should return an int/object indicating errors.
    // potential errors: invalid print parameters, already pending, IO error
    private BisonPdfExporterCallback mResultCallback;
    private PrintAttributes mAttributes;
    private ParcelFileDescriptor mFd;


    /**
     * BisonPdfExporter callback used to call onWrite* callbacks in Android framework.
     */
    public interface BisonPdfExporterCallback {
        /**
         * Called by the native side when PDF generation is done.
         * @param pageCount How many pages native side wrote to PDF file descriptor. Non-positive
         *                  value indicates native side writing failed.
         */
        public void pdfWritingDone(int pageCount);
    }

    BisonPdfExporter() {
    }


    public void exportToPdf(final ParcelFileDescriptor fd, PrintAttributes attributes, int[] pages,
            BisonPdfExporterCallback resultCallback, CancellationSignal cancellationSignal) {
        if (fd == null) {
            throw new IllegalArgumentException("fd cannot be null");
        }
        if (resultCallback == null) {
            throw new IllegalArgumentException("resultCallback cannot be null");
        }
        if (mResultCallback != null) {
            throw new IllegalStateException("printing is already pending");
        }
        if (attributes.getMediaSize() == null) {
            throw new  IllegalArgumentException("attributes must specify a media size");
        }
        if (attributes.getResolution() == null) {
            throw new IllegalArgumentException("attributes must specify print resolution");
        }
        if (attributes.getMinMargins() == null) {
            throw new IllegalArgumentException("attributes must specify margins");
        }
        if (mNativeBisonPdfExporter == 0) {
            resultCallback.pdfWritingDone(0);
            return;
        }
        mResultCallback = resultCallback;
        mAttributes = attributes;
        mFd = fd;
        BisonPdfExporterJni.get().exportToPdf(
                mNativeBisonPdfExporter, BisonPdfExporter.this, mFd.getFd(), pages, cancellationSignal);
    }

    @CalledByNative
    private void setNativeBisonPdfExporter(long nativePdfExporter) {
        mNativeBisonPdfExporter = nativePdfExporter;
        // Handle the cornercase that the native side is destroyed (for example
        // via Webview.Destroy) before it has a chance to complete the pdf exporting.
        if (nativePdfExporter == 0 && mResultCallback != null) {
            try {
                mResultCallback.pdfWritingDone(0);
                mResultCallback = null;
            } catch (IllegalStateException ex) {
                // Swallow the illegal state exception here. It is possible that app
                // is going away and binder is already finalized. b/25462345
            }
        }
    }

    private static int getPrintDpi(PrintAttributes attributes) {
        // TODO(sgurun) android print attributes support horizontal and
        // vertical DPI. Chrome has only one DPI. Revisit this.
        int horizontalDpi = attributes.getResolution().getHorizontalDpi();
        int verticalDpi = attributes.getResolution().getVerticalDpi();
        if (horizontalDpi != verticalDpi) {
            Log.w(TAG, "Horizontal and vertical DPIs differ. Using horizontal DPI "
                    + " hDpi=" + horizontalDpi + " vDPI=" + verticalDpi);
        }
        return horizontalDpi;
    }

    @CalledByNative
    private void didExportPdf(int pageCount) {
        mResultCallback.pdfWritingDone(pageCount);
        mResultCallback = null;
        mAttributes = null;
        // The caller should close the file.
        mFd = null;
    }

    @CalledByNative
    private int getPageWidth() {
        return mAttributes.getMediaSize().getWidthMils();
    }

    @CalledByNative
    private int getPageHeight() {
        return mAttributes.getMediaSize().getHeightMils();
    }

    @CalledByNative
    private int getDpi() {
        return getPrintDpi(mAttributes);
    }

    @CalledByNative
    private int getLeftMargin() {
        return mAttributes.getMinMargins().getLeftMils();
    }

    @CalledByNative
    private int getRightMargin() {
        return mAttributes.getMinMargins().getRightMils();
    }

    @CalledByNative
    private int getTopMargin() {
        return mAttributes.getMinMargins().getTopMils();
    }

    @CalledByNative
    private int getBottomMargin() {
        return mAttributes.getMinMargins().getBottomMils();
    }

    @NativeMethods
    interface Natives {
        void exportToPdf(long nativeBisonPdfExporter, BisonPdfExporter caller, int fd, int[] pages,
                         CancellationSignal cancellationSignal);
    }
}
