package im.shimo.bison;

public abstract class RenderProcessGoneDetail {

    public RenderProcessGoneDetail() {}

    public abstract boolean didCrash();

    /**
     * Returns the renderer priority that was set at the time that the
     * renderer exited.  This may be greater than the priority that
     * any individual {@link WebView} requested using
     * {@link WebView#setRendererPriorityPolicy}.
     *
     * @return the priority of the renderer at exit.
     **/
    public abstract int rendererPriorityAtExit();

}
