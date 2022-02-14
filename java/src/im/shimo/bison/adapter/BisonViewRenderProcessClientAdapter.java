package im.shimo.bison.adapter;

import im.shimo.bison.BisonView;
import im.shimo.bison.BisonViewRenderProcess;
import im.shimo.bison.BisonViewRenderProcessClient;
import im.shimo.bison.RenderProcessGoneDetail;
import im.shimo.bison.internal.BvRenderProcess;

import java.util.concurrent.Executor;

public class BisonViewRenderProcessClientAdapter {

    private Executor mExecutor;
    private BisonViewRenderProcessClient mRenderProcessClient;

    public BisonViewRenderProcessClientAdapter(Executor executor, BisonViewRenderProcessClient renderProcessClient) {
        mExecutor = executor;
        mRenderProcessClient = renderProcessClient;
    }

    public BisonViewRenderProcessClient getWebViewRenderProcessClient() {
        return mRenderProcessClient;
    }

    public void onRendererUnresponsive(final BisonView view, final BvRenderProcess renderProcess) {
        BisonViewRenderProcess renderer = BisonViewRenderProcessAdapter.getInstanceFor(renderProcess);
        mExecutor.execute(() -> mRenderProcessClient.onRenderProcessUnresponsive(view, renderer));
    }

    public void onRendererResponsive(final BisonView view, final BvRenderProcess renderProcess) {
        BisonViewRenderProcess renderer = BisonViewRenderProcessAdapter.getInstanceFor(renderProcess);
        mExecutor.execute(() -> mRenderProcessClient.onRenderProcessResponsive(view, renderer));
    }

    public boolean onRenderProcessGone(final BisonView view, final RenderProcessGoneDetail renderProcessGoneDetail) {
        return mRenderProcessClient.onRenderProcessGone(view, renderProcessGoneDetail);
    }

}
