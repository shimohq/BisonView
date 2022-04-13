package im.shimo.bison.adapter;

import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Paint;
import android.graphics.Picture;
import android.graphics.Rect;
import android.net.Uri;
import android.net.http.SslCertificate;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.print.PrintDocumentAdapter;
import android.util.SparseArray;
import android.view.DragEvent;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStructure;
import android.view.accessibility.AccessibilityNodeProvider;
import android.view.autofill.AutofillValue;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.textclassifier.TextClassifier;

import org.chromium.base.ThreadUtils;
import org.chromium.base.task.PostTask;
import org.chromium.content_public.browser.NavigationHistory;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.url.GURL;

import java.util.Map;
import java.util.concurrent.Callable;
import java.util.concurrent.Executor;

import im.shimo.bison.BisonInitializer;
import im.shimo.bison.BisonView;
import im.shimo.bison.BisonView.HitTestResult;
import im.shimo.bison.BisonView.InternalAccess;
import im.shimo.bison.BisonView.VisualStateCallback;
import im.shimo.bison.BisonViewClient;
import im.shimo.bison.BisonViewPrintDocumentAdapter;
import im.shimo.bison.BisonViewRenderProcess;
import im.shimo.bison.BisonViewRenderProcessClient;
import im.shimo.bison.BisonViewSettings;
import im.shimo.bison.BisonWebChromeClient;
import im.shimo.bison.ValueCallback;
import im.shimo.bison.WebBackForwardList;
import im.shimo.bison.WebMessage;
import im.shimo.bison.WebMessagePort;
import im.shimo.bison.internal.BvContents;
import im.shimo.bison.internal.BvContentsClientBridge;
import im.shimo.bison.internal.BvRunQueue;
import im.shimo.bison.internal.BvSettings;
import im.shimo.bison.internal.ClientCertLookupTable;
import im.shimo.bison.internal.ScriptReference;

public class BisonViewProvider {

    private BisonViewContentsClientAdapter mContentsClient;
    private BisonViewSettingsAdapter mSettings;
    private BvContents mBvContents;
    private InternalAccess mBisonViewInternalAccess;

    private final HitTestResult mHitTestResult;
    private final BvRunQueue mFactory;
    private BisonView mBisonView;

    public BisonViewProvider(BisonView view, int webContentsRenderView, InternalAccess internalAccess) {
        this.mBisonView = view;
        BisonInitializer.getInstance().ensureStarted();
        mFactory = BisonInitializer.getInstance().getRunQueue();
        mHitTestResult = new HitTestResult();
        this.mBisonViewInternalAccess = internalAccess;
        Context context = view.getContext();
        int appTargetSdkVersion = context.getApplicationInfo().targetSdkVersion;

        mContentsClient = new BisonViewContentsClientAdapter(view, context);
        BvSettings bvSettings = new BvSettings(context);
        if (appTargetSdkVersion >= Build.VERSION_CODES.P) {
            bvSettings.setCSSHexAlphaColorEnabled(true);
            bvSettings.setScrollTopLeftInteropEnabled(true);
        }
        mSettings = new BisonViewSettingsAdapter(bvSettings);
        BvContentsClientBridge bvContentsClientBridge = new BvContentsClientBridge(context, mContentsClient, new ClientCertLookupTable());

        mBvContents = new BvContents(context, view, BisonInitializer.getInstance().getBrowserContext(),
                new InternalAccessAdapter(), bvContentsClientBridge, mContentsClient, bvSettings, webContentsRenderView);
        BvContents.setShouldDownloadFavicons();
    }

    protected boolean checkNeedsPost() {
        return !ThreadUtils.runningOnUiThread();
    }

    private void checkThread() {
        if (!ThreadUtils.runningOnUiThread()) {
            throw new IllegalStateException("Calling View methods on another thread than the UI thread.");
        }
    }

    public void setHorizontalScrollbarOverlay(final boolean overlay) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    setHorizontalScrollbarOverlay(overlay);
                }
            });
            return;
        }
        mBvContents.setHorizontalScrollbarOverlay(overlay);
    }

    public void setVerticalScrollbarOverlay(boolean overlay) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    setVerticalScrollbarOverlay(overlay);
                }
            });
            return;
        }
        mBvContents.setVerticalScrollbarOverlay(overlay);
    }

    // overlayHorizontalScrollbar

    // overlayVerticalScrollbar

    // getVisibleTitleHeight no-op
    public SslCertificate getCertificate() {
        if (checkNeedsPost()) {
            SslCertificate ret = mFactory.runOnUiThreadBlocking(new Callable<SslCertificate>() {
                @Override
                public SslCertificate call() {
                    return getCertificate();
                }
            });
            return ret;
        }
        return mBvContents.getCertificate();
    }

    // setCertificate no-op
    // savePassword no-op

    // jiang setHttpAuthUsernamePassword
    // jiang getHttpAuthUsernamePassword

    public void destroy() {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    destroy();
                }
            });
            return;
        }

        setBisonWebChromeClient(null);
        setBisonViewClient(null);
        setFindListener(null);
        setDownloadListener(null);
        mBisonView = null;
        mBvContents.destroy();
        mBisonViewInternalAccess = null;
        mContentsClient = null;
    }

    public void setNetworkAvailable(final boolean networkUp) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    setNetworkAvailable(networkUp);
                }
            });
            return;
        }
        mBvContents.setNetworkAvailable(networkUp);
    }

    public WebBackForwardList saveState(final Bundle outState) {
        if (checkNeedsPost()) {
            WebBackForwardList ret = mFactory.runOnUiThreadBlocking(new Callable<WebBackForwardList>() {
                @Override
                public WebBackForwardList call() {
                    return saveState(outState);
                }
            });
            return ret;
        }
        if (outState == null)
            return null;
        if (!mBvContents.saveState(outState))
            return null;
        return copyBackForwardList();
    }

    // savePicture no-op

    // restorePicture no-op

    public WebBackForwardList restoreState(final Bundle inState) {
        if (checkNeedsPost()) {
            WebBackForwardList ret = mFactory.runOnUiThreadBlocking(new Callable<WebBackForwardList>() {
                @Override
                public WebBackForwardList call() {
                    return restoreState(inState);
                }
            });
            return ret;
        }
        if (inState == null)
            return null;
        if (!mBvContents.restoreState(inState))
            return null;
        return copyBackForwardList();
    }

    public void loadUrl(final String url, final Map<String, String> additionalHttpHeaders) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    mBvContents.loadUrl(url, additionalHttpHeaders);
                }
            });
            return;
        }
        mBvContents.loadUrl(url, additionalHttpHeaders);
    }

    public void loadUrl(final String url) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    mBvContents.loadUrl(url);
                }
            });
            return;
        }
        mBvContents.loadUrl(url);
    }

    public void postUrl(final String url, final byte[] postData) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    mBvContents.postUrl(url, postData);
                }
            });
            return;
        }
        mBvContents.postUrl(url, postData);
    }

    public void loadData(final String data, final String mimeType, final String encoding) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    mBvContents.loadData(data, mimeType, encoding);
                }
            });
            return;
        }
        mBvContents.loadData(data, mimeType, encoding);
    }

    public void loadDataWithBaseURL(String baseUrl, String data, String mimeType, String encoding, String historyUrl) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    mBvContents.loadDataWithBaseURL(baseUrl, data, mimeType, encoding, historyUrl);
                }
            });
            return;
        }
        mBvContents.loadDataWithBaseURL(baseUrl, data, mimeType, encoding, historyUrl);
    }

    public void evaluateJavaScript(String script, final ValueCallback<String> valueCallback) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    mBvContents.evaluateJavaScript(script, CallbackConverter.fromValueCallback(valueCallback));
                }
            });
            return;
        }
        mBvContents.evaluateJavaScript(script, CallbackConverter.fromValueCallback(valueCallback));
    }

    public void saveWebArchive(String filename) {
        saveWebArchive(filename, false, null);
    }

    public void saveWebArchive(final String basename, final boolean autoname,
                               final ValueCallback<String> valueCallback) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    saveWebArchive(basename, autoname, valueCallback);
                }
            });
            return;
        }
        mBvContents.saveWebArchive(basename, autoname, CallbackConverter.fromValueCallback(valueCallback));
    }

    public void stopLoading() {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    stopLoading();
                }
            });
            return;
        }
        mBvContents.stopLoading();
    }

    public void reload() {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    reload();
                }
            });
            return;
        }
        mBvContents.reload();
    }

    public boolean canGoBack() {
        if (checkNeedsPost()) {
            Boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return canGoBack();
                }
            });
            return ret;
        }
        return mBvContents.canGoBack();
    }

    public void goBack() {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    goBack();
                }
            });
            return;
        }
        mBvContents.goBack();
    }

    public boolean canGoForward() {
        if (checkNeedsPost()) {
            Boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return canGoForward();
                }
            });
            return ret;
        }
        return mBvContents.canGoForward();
    }

    public void goForward() {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    goForward();
                }
            });
            return;
        }
        mBvContents.goForward();
    }

    public boolean canGoBackOrForward(int steps) {
        if (checkNeedsPost()) {
            Boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return canGoBackOrForward(steps);
                }
            });
            return ret;
        }
        return mBvContents.canGoBackOrForward(steps);
    }

    public void goBackOrForward(final int steps) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    goBackOrForward(steps);
                }
            });
            return;
        }
        mBvContents.goBackOrForward(steps);
    }

    // isPrivateBrowsingEnabled

    // pageUp

    // pageDown

    public void insertVisualStateCallback(long requestId, VisualStateCallback callback) {
        mBvContents.insertVisualStateCallback(requestId,
                callback == null ? null : new BvContents.VisualStateCallback() {
                    @Override
                    public void onComplete(long requestId) {
                        callback.onComplete(requestId);
                    }
                });
    }

    // clearView

    public Picture capturePicture() {
        if (checkNeedsPost()) {
            Picture ret = mFactory.runOnUiThreadBlocking(new Callable<Picture>() {
                @Override
                public Picture call() {
                    return capturePicture();
                }
            });
            return ret;
        }
        return mBvContents.capturePicture();
    }

    // getScale

    // setInitialScale

    // invokeZoomPicker

    public HitTestResult getHitTestResult() {
        if (checkNeedsPost()) {
            BisonView.HitTestResult ret = mFactory.runOnUiThreadBlocking(new Callable<BisonView.HitTestResult>() {
                @Override
                public BisonView.HitTestResult call() {
                    return getHitTestResult();
                }
            });
            return ret;
        }
        BvContents.HitTestData data = mBvContents.getLastHitTestResult();
        mHitTestResult.setType(data.hitTestResultType);
        mHitTestResult.setExtra(data.hitTestResultExtraData);
        return mHitTestResult;
    }

    public void requestFocusNodeHref(final Message msg) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    requestFocusNodeHref(msg);
                }
            });
            return;
        }
        mBvContents.requestFocusNodeHref(msg);
    }

    public void requestImageRef(Message msg) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    requestImageRef(msg);
                }
            });
            return;
        }
        mBvContents.requestImageRef(msg);
    }

    public String getUrl() {
        if (checkNeedsPost()) {
            String ret = mFactory.runOnUiThreadBlocking(new Callable<String>() {
                @Override
                public String call() {
                    return getUrl();
                }
            });
            return ret;
        }
        GURL url = mBvContents.getUrl();
        return url == null ? null : url.getSpec();
    }

    public String getOriginalUrl() {
        if (checkNeedsPost()) {
            String ret = mFactory.runOnUiThreadBlocking(new Callable<String>() {
                @Override
                public String call() {
                    return getOriginalUrl();
                }
            });
            return ret;
        }
        return mBvContents.getOriginalUrl();
    }

    public String getTitle() {
        if (checkNeedsPost()) {
            String ret = mFactory.runOnUiThreadBlocking(new Callable<String>() {
                @Override
                public String call() {
                    return getTitle();
                }
            });
            return ret;
        }
        return mBvContents.getTitle();
    }

    public Bitmap getFavicon() {
        if (checkNeedsPost()) {
            Bitmap ret = mFactory.runOnUiThreadBlocking(new Callable<Bitmap>() {
                @Override
                public Bitmap call() {
                    return getFavicon();
                }
            });
            return ret;
        }
        return mBvContents.getFavicon();
    }

    // getTouchIconUrl no-op

    public int getProgress() {
        if (mBvContents == null)
            return 100;
        return mBvContents.getMostRecentProgress();
    }

    // getContentHeight
    // getContentWidth
    // public int getContentHeightCss() {
    // return mBvContents.getContentHeightCss();
    // }

    // public int getContentWidthCss() {
    // return mBvContents.getContentWidthCss();
    // }

    public void pauseTimers() {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    pauseTimers();
                }
            });
            return;
        }
        mBvContents.pauseTimers();
    }

    public void resumeTimers() {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    resumeTimers();
                }
            });
            return;
        }
        mBvContents.resumeTimers();
    }

    public void onPause() {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    onPause();
                }
            });
            return;
        }
        mBvContents.onPause();
    }

    public void onResume() {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    onResume();
                }
            });
            return;
        }
        mBvContents.onResume();
    }

    public boolean isPaused() {
        if (checkNeedsPost()) {
            Boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return isPaused();
                }
            });
            return ret;
        }
        return mBvContents.isPaused();
    }

    // freeMemory on-op

    public void clearCache(boolean includeDiskFiles) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    clearCache(includeDiskFiles);
                }
            });
            return;
        }
        mBvContents.clearCache(includeDiskFiles);
    }

    public void clearFormData() {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    clearFormData();
                }
            });
            return;
        }
        mBvContents.hideAutofillPopup();
    }

    public void clearHistory() {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    clearHistory();
                }
            });
            return;
        }
        mBvContents.clearHistory();
    }

    public void clearSslPreferences() {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    clearSslPreferences();
                }
            });
            return;
        }
        mBvContents.clearSslPreferences();
    }

    public WebBackForwardList copyBackForwardList() {
        if (checkNeedsPost()) {
            WebBackForwardList ret = mFactory.runOnUiThreadBlocking(new Callable<WebBackForwardList>() {
                @Override
                public WebBackForwardList call() {
                    return copyBackForwardList();
                }
            });
            return ret;
        }
        NavigationHistory navHistory = mBvContents.getNavigationHistory();
        if (navHistory == null)
            navHistory = new NavigationHistory();
        return new WebBackForwardListAdapter(navHistory);
    }

    public void setFindListener(BisonView.FindListener listener) {
        mContentsClient.setFindListener(listener);
    }

    public void findNext(boolean forward) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    findNext(forward);
                }
            });
            return;
        }
        mBvContents.findNext(forward);
    }

    public int findAll(final String searchString) {
        findAllAsync(searchString);
        return 0;
    }

    public void findAllAsync(String searchString) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    findAllAsync(searchString);
                }
            });
            return;
        }
        mBvContents.findAllAsync(searchString);
    }

    // jiang showFindDialog

    public void notifyFindDialogDismissed() {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    notifyFindDialogDismissed();
                }
            });
            return;
        }
        clearMatches();
    }

    public void clearMatches() {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    clearMatches();
                }
            });
            return;
        }
        mBvContents.clearMatches();
    }

    public void documentHasImages(Message message) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    documentHasImages(message);
                }
            });
            return;
        }
        mBvContents.documentHasImages(message);
    }

    public void setBisonViewClient(BisonViewClient client) {
        mContentsClient.setBisonViewClient(client);
    }

    public BisonViewClient getBisonViewClient() {
        return mContentsClient.getBisonViewClient();
    }

    public BisonViewRenderProcess getBisonViewRenderProcess() {
        if (checkNeedsPost()) {
            return mFactory.runOnUiThreadBlocking(() -> getBisonViewRenderProcess());
        }
        return BisonViewRenderProcessAdapter.getInstanceFor(mBvContents.getRenderProcess());
    }

    public void setBisonViewRenderProcessClient(Executor executor, BisonViewRenderProcessClient client) {
        if (client == null) {
            mContentsClient.setBisonViewRenderProcessClientAdapter(null);
        } else {
            if (executor == null) {
                executor = (Runnable r) -> r.run();
            }
            BisonViewRenderProcessClientAdapter adapter = new BisonViewRenderProcessClientAdapter(executor, client);
            mContentsClient.setBisonViewRenderProcessClientAdapter(adapter);
        }

    }

    public BisonViewRenderProcessClient getBisonViewRenderProcessClient() {
        BisonViewRenderProcessClientAdapter adapter = mContentsClient.getBisonViewRendererClientAdapter();
        if (adapter != null) {
            return adapter.getWebViewRenderProcessClient();
        } else {
            return null;
        }

    }

    public void setDownloadListener(BisonView.DownloadListener listener) {
        mContentsClient.setDownloadListener(listener);
    }

    public void setBisonWebChromeClient(BisonWebChromeClient client) {
        mContentsClient.setBisonWebChromeClient(client);
    }

    public BisonWebChromeClient getBisonWebChromeClient() {
        return mContentsClient.getBisonWebChromeClient();
    }

    // doesSupportFullscreen

    // setPictureListener

    public void addJavascriptInterface(Object object, String name) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    addJavascriptInterface(object, name);
                }
            });
            return;
        }
        mBvContents.addJavascriptInterface(object, name);
    }

    public void removeJavascriptInterface(String interfaceName) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    removeJavascriptInterface(interfaceName);
                }
            });
            return;
        }
        mBvContents.removeJavascriptInterface(interfaceName);
    }

    public WebMessagePort[] createWebMessageChannel() {
        if (checkNeedsPost()) {
            return mFactory.runOnUiThreadBlocking(this::createWebMessageChannel);
        }

        return WebMessagePortAdapter.fromMessagePorts(mBvContents.createMessageChannel());
    }

    public void postMessageToMainFrame(final WebMessage message, final Uri targetOrigin) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    postMessageToMainFrame(message, targetOrigin);
                }
            });
        }
        mBvContents.postMessageToMainFrame(message.getData(), targetOrigin.toString(),
                WebMessagePortAdapter.toMessagePorts(message.getPorts()));
    }

    public BisonViewSettings getSettings() {
        return mSettings;
    }

    // setMapTrackballToArrowKeys intentional no-op.
    // flingScroll
    // getZoomControls

    public boolean canZoomIn() {
        if (checkNeedsPost()) {
            return false;
        }
        return mBvContents.canZoomIn();
    }

    public boolean canZoomOut() {
        if (checkNeedsPost()) {
            return false;
        }
        return mBvContents.canZoomOut();
    }

    public boolean zoomIn() {
        if (checkNeedsPost()) {
            boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return zoomIn();
                }
            });
            return ret;
        }
        return mBvContents.zoomIn();
    }

    public boolean zoomOut() {
        if (checkNeedsPost()) {
            boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return zoomOut();
                }
            });
            return ret;
        }
        return mBvContents.zoomOut();
    }

    public void zoomBy(float factor) {
        checkThread();
        mBvContents.zoomBy(factor);
    }

    // dumpViewHierarchyWithProperties no-op

    // findHierarchyView no-op

    public void setRendererPriorityPolicy(int rendererRequestedPriority, boolean waivedWhenNotVisible) {
        mBvContents.setRendererPriorityPolicy(rendererRequestedPriority, waivedWhenNotVisible);
    }

    // jiang im.shimo.bison.internal.RendererPriority convert to BisonView
    public int getRendererRequestedPriority() {
        return mBvContents.getRendererRequestedPriority();
    }

    public boolean getRendererPriorityWaivedWhenNotVisible() {
        return mBvContents.getRendererPriorityWaivedWhenNotVisible();
    }

    public void setTextClassifier(TextClassifier textClassifier) {
        mBvContents.setTextClassifier(textClassifier);
    }

    public TextClassifier getTextClassifier() {
        return mBvContents.getTextClassifier();
    }

    public void autofill(SparseArray<AutofillValue> values) {
        if (checkNeedsPost()) {
            mFactory.runVoidTaskOnUiThreadBlocking(new Runnable() {
                @Override
                public void run() {
                    autofill(values);
                }
            });
        }
        mBvContents.autofill(values);
    }

    public void onProvideAutoFillVirtualStructure(final ViewStructure structure, final int flags) {
        if (checkNeedsPost()) {
            mFactory.runVoidTaskOnUiThreadBlocking(new Runnable() {
                @Override
                public void run() {
                    onProvideAutoFillVirtualStructure(structure, flags);
                }
            });
            return;
        }
        mBvContents.onProvideAutoFillVirtualStructure(structure, flags);
    }

    // onProvideContentCaptureStructure

    // shouldDelayChildPressedState

    public AccessibilityNodeProvider getAccessibilityNodeProvider() {
        if (checkNeedsPost()) {
            AccessibilityNodeProvider ret = mFactory.runOnUiThreadBlocking(new Callable<AccessibilityNodeProvider>() {
                @Override
                public AccessibilityNodeProvider call() {
                    return getAccessibilityNodeProvider();
                }
            });
            return ret;
        }
        return mBvContents.getAccessibilityNodeProvider();
    }

    public void onProvideVirtualStructure(final ViewStructure structure) {
        if (checkNeedsPost()) {
            mFactory.runVoidTaskOnUiThreadBlocking(new Runnable() {
                @Override
                public void run() {
                    onProvideVirtualStructure(structure);
                }
            });
            return;
        }
        mBvContents.onProvideVirtualStructure(structure);
    }

    // onInitializeAccessibilityNodeInfo

    // onInitializeAccessibilityEvent

    public boolean performAccessibilityAction(final int action, final Bundle arguments) {
        if (mBisonViewInternalAccess == null) return false;
        if (checkNeedsPost()) {
            boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return performAccessibilityAction(action, arguments);
                }
            });
            return ret;
        }
        if (mBvContents.supportsAccessibilityAction(action)) {
            return mBvContents.performAccessibilityAction(action, arguments);
        }
        return mBisonViewInternalAccess.super_performAccessibilityAction(action, arguments);
    }

    public void setOverScrollMode(final int mode) {
        if (mBvContents == null)
            return;
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    setOverScrollMode(mode);
                }
            });
            return;
        }
        mBvContents.setOverScrollMode(mode);
    }

    public void setScrollBarStyle(final int style) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    setScrollBarStyle(style);
                }
            });
            return;
        }
        mBvContents.setScrollBarStyle(style);
    }

    // onDrawVerticalScrollBar

    public void onOverScrolled(final int scrollX, final int scrollY, final boolean clampedX, final boolean clampedY) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    onOverScrolled(scrollX, scrollY, clampedX, clampedY);
                }
            });
            return;
        }
        mBvContents.onContainerViewOverScrolled(scrollX, scrollY, clampedX, clampedY);
    }

    public void onWindowVisibilityChanged(int visibility) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    onWindowVisibilityChanged(visibility);
                }
            });
            return;
        }
        mBvContents.onWindowVisibilityChanged(visibility);
    }

    // onDraw

    public void setLayoutParams(final ViewGroup.LayoutParams layoutParams) {
        checkThread();
        if (mBisonViewInternalAccess == null) return;
        mBisonViewInternalAccess.super_setLayoutParams(layoutParams);
        if (checkNeedsPost()) {
            mFactory.runVoidTaskOnUiThreadBlocking(new Runnable() {
                @Override
                public void run() {
                    mBvContents.setLayoutParams(layoutParams);
                }
            });
            return;
        }
        mBvContents.setLayoutParams(layoutParams);
    }

    public void onActivityResult(final int requestCode, final int resultCode, final Intent data) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    onActivityResult(requestCode, resultCode, data);
                }
            });
            return;
        }
        mBvContents.onActivityResult(requestCode, resultCode, data);
    }

    public boolean performLongClick() {
        if (mBisonView == null || mBisonViewInternalAccess == null) {
            return false;
        }
        return mBisonView.getParent() != null ? mBisonViewInternalAccess.super_performLongClick() : false;
    }

    public void onConfigurationChanged(final Configuration newConfig) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    onConfigurationChanged(newConfig);
                }
            });
            return;
        }
        mBvContents.onConfigurationChanged(newConfig);
    }

    public boolean onDragEvent(final DragEvent event) {
        if (checkNeedsPost()) {
            boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return onDragEvent(event);
                }
            });
            return ret;
        }
        return mBvContents.onDragEvent(event);
    }

    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        if (checkNeedsPost()) {
            return null;
        }
        return mBvContents.onCreateInputConnection(outAttrs);
    }

    public boolean onKeyMultiple(final int keyCode, final int repeatCount, final KeyEvent event) {
        if (checkNeedsPost()) {
            boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return onKeyMultiple(keyCode, repeatCount, event);
                }
            });
            return ret;
        }
        return false;
    }

    public boolean onKeyDown(final int keyCode, final KeyEvent event) {
        if (checkNeedsPost()) {
            boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return onKeyDown(keyCode, event);
                }
            });
            return ret;
        }
        return false;
    }

    public boolean onKeyUp(final int keyCode, final KeyEvent event) {
        if (checkNeedsPost()) {
            boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return onKeyUp(keyCode, event);
                }
            });
            return ret;
        }
        return mBvContents.onKeyUp(keyCode, event);
    }

    public void onAttachedToWindow() {
        checkThread();
        mBvContents.onAttachedToWindow();
    }

    public void onDetachedFromWindow() {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    onDetachedFromWindow();
                }
            });
            return;
        }
        mBvContents.onDetachedFromWindow();
    }

    public void onVisibilityChanged(final View changedView, final int visibility) {
        if (mBvContents == null)
            return;

        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    onVisibilityChanged(changedView, visibility);
                }
            });
            return;
        }
        mBvContents.onVisibilityChanged(changedView, visibility);
    }

    public void onWindowFocusChanged(final boolean hasWindowFocus) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    onWindowFocusChanged(hasWindowFocus);
                }
            });
            return;
        }
        mBvContents.onWindowFocusChanged(hasWindowFocus);
    }

    public void onFocusChanged(final boolean focused, final int direction, final Rect previouslyFocusedRect) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    onFocusChanged(focused, direction, previouslyFocusedRect);
                }
            });
            return;
        }
        mBvContents.onFocusChanged(focused, direction, previouslyFocusedRect);
    }

    // jiang setFrame 是view的hide 方法，后面研究是否要有必要实现
    // public boolean setFrame(final int left, final int top, final int right, final
    // int bottom) {
    // return mBisonViewInternalAccess.super_setFrame(left, top, right, bottom);
    // }

    public void onSizeChanged(int w, int h, int ow, int oh) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    onSizeChanged(w, h, ow, oh);
                }
            });
            return;
        }
        mBvContents.onSizeChanged(w, h, ow, oh);
    }

    public void onScrollChanged(final int l, final int t, final int oldl, final int oldt) {
        if (checkNeedsPost()) {
            mFactory.addTask(new Runnable() {
                @Override
                public void run() {
                    onScrollChanged(l, t, oldl, oldt);
                }
            });
            return;
        }

        //mBvContents.onContainerViewScrollChanged(l, t, oldl, oldt);
    }

    public boolean dispatchKeyEvent(final KeyEvent event) {
        if (checkNeedsPost()) {
            boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return dispatchKeyEvent(event);
                }
            });
            return ret;
        }
        return mBvContents.dispatchKeyEvent(event);
    }

    public boolean onTouchEvent(final MotionEvent event) {
        if (checkNeedsPost()) {
            boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return onTouchEvent(event);
                }
            });
            return ret;
        }
        return mBvContents.onTouchEvent(event);
    }

    public boolean onHoverEvent(final MotionEvent event) {
        if (checkNeedsPost()) {
            boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return onHoverEvent(event);
                }
            });
            return ret;
        }
        return mBvContents.onHoverEvent(event);
    }

    public boolean onGenericMotionEvent(final MotionEvent event) {
        if (checkNeedsPost()) {
            boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return onGenericMotionEvent(event);
                }
            });
            return ret;
        }
        return mBvContents.onGenericMotionEvent(event);
    }

    // onTrackballEvent

    public boolean requestFocus(final int direction, final Rect previouslyFocusedRect) {
        if (checkNeedsPost()) {
            boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return requestFocus(direction, previouslyFocusedRect);
                }
            });
            return ret;
        }
        mBvContents.requestFocus();
        return mBisonViewInternalAccess.super_requestFocus(direction, previouslyFocusedRect);
    }

    public void onMeasure(final int widthMeasureSpec, final int heightMeasureSpec) {
        if (checkNeedsPost()) {
            mFactory.runVoidTaskOnUiThreadBlocking(new Runnable() {
                @Override
                public void run() {
                    onMeasure(widthMeasureSpec, heightMeasureSpec);
                }
            });
            return;
        }
        mBvContents.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    public boolean requestChildRectangleOnScreen(final View child, final Rect rect, final boolean immediate) {
        if (checkNeedsPost()) {
            boolean ret = mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return requestChildRectangleOnScreen(child, rect, immediate);
                }
            });
            return ret;
        }
        return mBvContents.requestChildRectangleOnScreen(child, rect, immediate);
    }

    public void setBackgroundColor(final int color) {
        if (checkNeedsPost()) {
            PostTask.postTask(UiThreadTaskTraits.DEFAULT, new Runnable() {
                @Override
                public void run() {
                    setBackgroundColor(color);
                }
            });
            return;
        }
        mBvContents.setBackgroundColor(color);
    }

    public void setLayerType(int layerType, Paint paint) {
        if (mBvContents == null)
            return;
        if (checkNeedsPost()) {
            PostTask.postTask(UiThreadTaskTraits.DEFAULT, new Runnable() {
                @Override
                public void run() {
                    setLayerType(layerType, paint);
                }
            });
            return;
        }
        mBvContents.setLayerType(layerType, paint);
    }

    // getHandler
    // findFocus
    // preDispatchDraw no-op
    // onStartTemporaryDetach
    // onFinishTemporaryDetach

    public boolean onCheckIsTextEditor() {
        if (checkNeedsPost()) {
            return mFactory.runOnUiThreadBlocking(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return onCheckIsTextEditor();
                }
            });
        }
        return mBvContents.onCheckIsTextEditor();
    }

    public void scrollBy(int x, int y) {
        mBvContents.scrollBy(x, y);
    }

    public void scrollTo(int x, int y) {
        mBvContents.scrollTo(x, y);
    }

    public int computeHorizontalScrollRange() {
        if (checkNeedsPost()) {
            int ret = mFactory.runOnUiThreadBlocking(new Callable<Integer>() {
                @Override
                public Integer call() {
                    return computeHorizontalScrollRange();
                }
            });
            return ret;
        }
        return mBvContents.computeHorizontalScrollRange();
    }

    public int computeHorizontalScrollOffset() {
        if (checkNeedsPost()) {
            int ret = mFactory.runOnUiThreadBlocking(new Callable<Integer>() {
                @Override
                public Integer call() {
                    return computeHorizontalScrollOffset();
                }
            });
            return ret;
        }
        return mBvContents.computeHorizontalScrollOffset();
    }

    public int computeHorizontalScrollExtent() {
        if (checkNeedsPost()) {
            int ret = mFactory.runOnUiThreadBlocking(new Callable<Integer>() {
                @Override
                public Integer call() {
                    return computeHorizontalScrollExtent();
                }
            });
            return ret;
        }
        return mBvContents.computeHorizontalScrollExtent();
    }

    public int computeVerticalScrollRange() {
        if (checkNeedsPost()) {
            int ret = mFactory.runOnUiThreadBlocking(new Callable<Integer>() {
                @Override
                public Integer call() {
                    return computeVerticalScrollRange();
                }
            });
            return ret;
        }
        return mBvContents.computeVerticalScrollRange();
    }

    public int computeVerticalScrollOffset() {
        if (checkNeedsPost()) {
            int ret = mFactory.runOnUiThreadBlocking(new Callable<Integer>() {
                @Override
                public Integer call() {
                    return computeVerticalScrollOffset();
                }
            });
            return ret;
        }
        return mBvContents.computeVerticalScrollOffset();
    }

    public int computeVerticalScrollExtent() {
        if (checkNeedsPost()) {
            int ret = mFactory.runOnUiThreadBlocking(new Callable<Integer>() {
                @Override
                public Integer call() {
                    return computeVerticalScrollExtent();
                }
            });
            return ret;
        }
        return mBvContents.computeVerticalScrollExtent();
    }

    public void computeScroll() {
        if (checkNeedsPost()) {
            mFactory.runVoidTaskOnUiThreadBlocking(new Runnable() {
                @Override
                public void run() {
                    computeScroll();
                }
            });
            return;
        }
        mBvContents.computeScroll();
    }

    public PrintDocumentAdapter createPrintDocumentAdapter(String documentName) {
        checkThread();
        return new BisonViewPrintDocumentAdapter(mBvContents.getPdfExporter(), documentName);
    }

    public void setWebContentsRenderView(int renderView) {
        if (checkNeedsPost()) {
            mFactory.runVoidTaskOnUiThreadBlocking(() -> setWebContentsRenderView(renderView));
            return;
        }
        mBvContents.setWebContentsRenderView(renderView);
    }

    private class InternalAccessAdapter implements BvContents.InternalAccessDelegate {

        @Override
        public boolean super_onKeyUp(int keyCode, KeyEvent event) {
            // jiang webview no-op
            return mBisonViewInternalAccess.super_onKeyUp(keyCode, event);
        }

        @Override
        public void onScrollChanged(int l, int t, int oldl, int oldt) {
            // jiang webview no-op ?
            mBisonViewInternalAccess.onScrollChanged(l, t, oldl, oldt);
        }

        @Override
        public boolean super_dispatchKeyEvent(KeyEvent event) {
            return mBisonViewInternalAccess.super_dispatchKeyEvent(event);
        }

        @Override
        public boolean super_onGenericMotionEvent(MotionEvent event) {
            return mBisonViewInternalAccess.super_onGenericMotionEvent(event);
        }

        @Override
        public void overScrollBy(int deltaX, int deltaY, int scrollX, int scrollY, int scrollRangeX, int scrollRangeY,
                                 int maxOverScrollX, int maxOverScrollY, boolean isTouchEvent) {
            mBisonViewInternalAccess.overScrollBy(deltaX, deltaY, scrollX, scrollY, scrollRangeX, scrollRangeY,
                    maxOverScrollX, maxOverScrollY, isTouchEvent);

        }

        @Override
        public void super_scrollTo(int scrollX, int scrollY) {
            mBisonViewInternalAccess.super_scrollTo(scrollX, scrollY);
        }

        @Override
        public void setMeasuredDimension(int measuredWidth, int measuredHeight) {
            mBisonViewInternalAccess.setMeasuredDimension(measuredWidth, measuredHeight);
        }

        @Override
        public int super_getScrollBarStyle() {
            return mBisonViewInternalAccess.super_getScrollBarStyle();
        }

        @Override
        public void super_startActivityForResult(Intent intent, int requestCode) {
            mBisonViewInternalAccess.super_startActivityForResult(intent, requestCode);
        }

        @Override
        public void super_onConfigurationChanged(Configuration newConfig) {
            // jiang sys webview no-op ?
            mBisonViewInternalAccess.super_onConfigurationChanged(newConfig);
        }

    }

    public void extractSmartClipData(int x, int y, int width, int height) {
        checkThread();
        mBvContents.extractSmartClipData(x, y, width, height);
    }

    public void setSmartClipResultHandler(Handler resultHandler) {
        checkThread();
        mBvContents.setSmartClipResultHandler(resultHandler);
    }

    public ScriptReference addDocumentStartJavaScript(String script, String[] allowedOriginRules) {
        return mBvContents.addDocumentStartJavaScript(script, allowedOriginRules);
    }

}
