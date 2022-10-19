package im.shimo.bison.test;

import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.InitializationError;

// import org.chromium.android_webview.common.AwSwitches;
// import org.chromium.android_webview.test.OnlyRunIn.ProcessMode;
import org.chromium.base.CommandLine;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.components.policy.test.annotations.Policies;


import java.util.ArrayList;
import java.util.List;

import androidx.annotation.CallSuper;

public final class BvJUnit4ClassRunner extends BaseJUnit4ClassRunner {

    // private final TestHook mBisonViewMultiProcessHook = (targetContext, testMethod) ->{
    //     // if (testMethod instanceof){

    //     // }
    // }


    public BvJUnit4ClassRunner (Class<?> klass) throws InitializationError {
        super(klass);
    }

    @CallSuper
    @Override
    protected List<TestHook> getPreTestHooks() {
        return addToList(
                super.getPreTestHooks(), Policies.getRegistrationHook());
    }


    @Override
    protected List<FrameworkMethod> getChildren() {
        List<FrameworkMethod> result = new ArrayList<>();
        result.add(new BisonViewMutilProcessFrameworkMethod(method));
        return result;
    }


    private static class BisonViewMutilProcessFrameworkMethod extends FrameworkMethod {

        public BisonViewMutilProcessFrameworkMethod (FrameworkMethod method){
            super(method.getMethod());
        }

        @Override
        public String getName() {
            return super.getName() + "__multiprocess_mode";
        }

        @Override
        public boolean equals(Object obj) {
            if (obj instanceof WebViewMultiProcessFrameworkMethod) {
                WebViewMultiProcessFrameworkMethod method =
                        (WebViewMultiProcessFrameworkMethod) obj;
                return super.equals(obj) && method.getName().equals(getName());
            }
            return false;
        }

        @Override
        public int hashCode() {
            int result = 17;
            result = 31 * result + super.hashCode();
            result = 31 * result + getName().hashCode();
            return result;
        }


    }


}
