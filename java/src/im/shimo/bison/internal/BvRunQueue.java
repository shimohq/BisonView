package im.shimo.bison.internal;

import org.chromium.base.ThreadUtils;
import org.chromium.base.task.PostTask;
import org.chromium.content_public.browser.UiThreadTaskTraits;

import java.util.Queue;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;

public class BvRunQueue {

    private final Queue<Runnable> mQueue;
    private final IsStartedCallable mHasStartedCallable;

    public static interface IsStartedCallable { public boolean isStarted(); }

    public BvRunQueue(IsStartedCallable callable){
        mQueue = new ConcurrentLinkedQueue<Runnable>();
        mHasStartedCallable = callable;
    }

    public void addTask(Runnable task) {
        mQueue.add(task);
        if (mHasStartedCallable.isStarted()) {
            PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> { drainQueue(); });
        }
    }

    public void drainQueue() {
        if (mQueue == null || mQueue.isEmpty()) {
            return;
        }

        Runnable task = mQueue.poll();
        while (task != null) {
            task.run();
            task = mQueue.poll();
        }
    }

    public boolean isStarted() {
        return mHasStartedCallable.isStarted();
    }


    public <T> T runBlockingFuture(FutureTask<T> task) {
        if (!isStarted()) throw new RuntimeException("Must be started before we block!");
        if (ThreadUtils.runningOnUiThread()) {
            throw new IllegalStateException("This method should only be called off the UI thread");
        }
        addTask(task);
        try {
            return task.get(4, TimeUnit.SECONDS);
        } catch (java.util.concurrent.TimeoutException e) {
            throw new RuntimeException("Probable deadlock detected due to WebView API being called "
                            + "on incorrect thread while the UI thread is blocked.",
                    e);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    // We have a 4 second timeout to try to detect deadlocks to detect and aid in debugging
    // deadlocks.
    // Do not call this method while on the UI thread!
    public void runVoidTaskOnUiThreadBlocking(Runnable r) {
        FutureTask<Void> task = new FutureTask<Void>(r, null);
        runBlockingFuture(task);
    }

    public <T> T runOnUiThreadBlocking(Callable<T> c) {
        return runBlockingFuture(new FutureTask<T>(c));
    }


}
