package im.shimo.bison;


import androidx.annotation.NonNull;


public interface BisonRenderProcessClient {

    default void onRenderProcessUnresponsive(@NonNull BisonView bisonView , @NonNull BisonRenderProcess render){}

    default void onRenderProcessResponsive(@NonNull BisonView bisonView , @NonNull BisonRenderProcess render){}

    default public boolean onRenderProcessGone(BisonView view, BisonRenderProcessGoneDetail detail) {
        return false;
    }
}