package im.shimo.bison.internal;


public class BvRenderProcessGoneDetail {

    private final boolean mDidCrash;
    @RendererPriority
    private final int mRendererPriority;

    public BvRenderProcessGoneDetail(boolean didCrash, @RendererPriority int rendererPriority) {
        mDidCrash = didCrash;
        mRendererPriority = rendererPriority;
    }

    public boolean didCrash() {
        return mDidCrash;
    }

    @RendererPriority
    public int rendererPriority() {
        return mRendererPriority;
    }
}
