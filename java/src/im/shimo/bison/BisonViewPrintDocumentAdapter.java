// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison;

import java.util.ArrayList;

import android.os.Bundle;
import android.os.CancellationSignal;
import android.os.ParcelFileDescriptor;
import android.print.PageRange;
import android.print.PrintAttributes;
import android.print.PrintDocumentAdapter;
import android.print.PrintDocumentInfo;
import im.shimo.bison.internal.BvPdfExporter;

/**
 * Adapter for printing BisonView. This class implements the abstract
 * system class PrintDocumentAdapter
 */
public class BisonViewPrintDocumentAdapter extends PrintDocumentAdapter {

    private BvPdfExporter mPdfExporter;
    private PrintAttributes mAttributes;
    private String mDocumentName;

    /**
     * Constructor.
     *
     * @param pdfExporter The PDF exporter to export the webview contents to a PDF file.
     */
    public BisonViewPrintDocumentAdapter(BvPdfExporter pdfExporter) {
        this(pdfExporter, "default");
    }

    /**
     * Constructor.
     *
     * @param pdfExporter The PDF exporter to export the webview contents to a PDF file.
     * @param documentName  The name of the pdf document.
     */
    public BisonViewPrintDocumentAdapter(BvPdfExporter pdfExporter, String documentName) {
        mPdfExporter = pdfExporter;
        mDocumentName = documentName;
    }


    @Override
    public void onLayout(PrintAttributes oldAttributes, PrintAttributes newAttributes,
            CancellationSignal cancellationSignal, LayoutResultCallback callback,
            Bundle metadata) {
        mAttributes = newAttributes;
        PrintDocumentInfo documentInfo = new PrintDocumentInfo
                .Builder(mDocumentName)
                .build();
        callback.onLayoutFinished(documentInfo, true);
    }

    @Override
    public void onWrite(final PageRange[] pages, ParcelFileDescriptor destination,
            CancellationSignal cancellationSignal, final WriteResultCallback callback) {
        if (pages == null || pages.length == 0) {
            callback.onWriteFailed(null);
            return;
        }

        mPdfExporter.exportToPdf(destination, mAttributes,
                normalizeRanges(pages), pageCount -> {
                    if (pageCount > 0) {
                        callback.onWriteFinished(validatePageRanges(pages, pageCount));
                    } else {
                        callback.onWriteFailed(null);
                    }
                }, cancellationSignal);
    }

    private static PageRange[] validatePageRanges(PageRange[] pages, int pageCount) {
        if (pages.length == 1 && PageRange.ALL_PAGES.equals(pages[0])) {
            return new PageRange[] {new PageRange(0, pageCount - 1)};
        }
        return pages;
    }

    private static int[] normalizeRanges(final PageRange[] ranges) {
        if (ranges.length == 1 && PageRange.ALL_PAGES.equals(ranges[0])) {
            return new int[0];
        }
        ArrayList<Integer> pages = new ArrayList<Integer>();
        for (PageRange range : ranges) {
            for (int i = range.getStart(); i <= range.getEnd(); ++i) {
                pages.add(i);
            }
        }

        int[] ret = new int[pages.size()];
        for (int i = 0; i < pages.size(); ++i) {
            ret[i] = pages.get(i).intValue();
        }
        return ret;
    }
}
