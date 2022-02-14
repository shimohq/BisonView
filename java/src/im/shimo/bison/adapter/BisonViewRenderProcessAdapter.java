package im.shimo.bison.adapter;

import java.lang.ref.WeakReference;
import java.util.WeakHashMap;

import androidx.annotation.RestrictTo;
import im.shimo.bison.BisonViewRenderProcess;
import im.shimo.bison.internal.BvRenderProcess;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public class BisonViewRenderProcessAdapter extends BisonViewRenderProcess {

    private static WeakHashMap<BvRenderProcess, BisonViewRenderProcessAdapter> sInstances =
            new WeakHashMap<>();

    private WeakReference<BvRenderProcess> mBvRenderProcessWeakRef;

    public static BisonViewRenderProcessAdapter getInstanceFor(BvRenderProcess bvRenderProcess) {
        if (bvRenderProcess == null) {
            return null;
        }
        BisonViewRenderProcessAdapter instance = sInstances.get(bvRenderProcess);
        if (instance == null) {
            sInstances.put(
                    bvRenderProcess, instance = new BisonViewRenderProcessAdapter(bvRenderProcess));
        }
        return instance;
    }

    private BisonViewRenderProcessAdapter(BvRenderProcess bvRenderProcess) {
        mBvRenderProcessWeakRef = new WeakReference<>(bvRenderProcess);
    }

    @Override
    public boolean terminate() {
        BvRenderProcess renderer = mBvRenderProcessWeakRef.get();
        if (renderer == null) {
            return false;
        }
        return renderer.terminate();
    }

}
