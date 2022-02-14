package im.shimo.bison;


import androidx.annotation.NonNull;


public interface BisonViewRenderProcessClient {

    default void onRenderProcessUnresponsive(@NonNull BisonView view , @NonNull BisonViewRenderProcess renderer){}

    default void onRenderProcessResponsive(@NonNull BisonView view , @NonNull BisonViewRenderProcess renderer){}

    default public boolean onRenderProcessGone(BisonView view, RenderProcessGoneDetail renderProcessGoneDetail) {
        return true;
    }
}
