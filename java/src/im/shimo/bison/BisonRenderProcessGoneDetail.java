package im.shimo.bison;

public class BisonRenderProcessGoneDetail {
    private final boolean mDidCrash;
    @RendererPriority
    private final int mRendererPriority;

    public BisonRenderProcessGoneDetail(boolean didCrash, @RendererPriority int rendererPriority) {
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
