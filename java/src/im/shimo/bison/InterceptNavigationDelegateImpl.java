package im.shimo.bison;

import org.chromium.components.navigation_interception.InterceptNavigationDelegate;
import org.chromium.components.navigation_interception.NavigationParams;

public class InterceptNavigationDelegateImpl implements InterceptNavigationDelegate {
    @Override
    public boolean shouldIgnoreNavigation(NavigationParams navigationParams) {

        return false;
    }
}
