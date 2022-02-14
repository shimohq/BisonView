package im.shimo.bison.adapter;

import android.os.Build;

import im.shimo.bison.BisonViewSettings;
import im.shimo.bison.internal.BvSettings;

public class BisonViewSettingsAdapter extends BisonViewSettings {

    private BvSettings mBvSettings;
    private PluginState mPluginState = PluginState.OFF;

    public BisonViewSettingsAdapter(BvSettings settings) {
        mBvSettings = settings;
    }

    BvSettings getBvSettings() {
        return mBvSettings;
    }

    @Override
    public void setSupportZoom(boolean support) {
        mBvSettings.setSupportZoom(support);
    }

    @Override
    public boolean supportZoom() {
        return mBvSettings.supportZoom();
    }

    @Override
    public void setBuiltInZoomControls(boolean enabled) {
        mBvSettings.setBuiltInZoomControls(enabled);
    }

    @Override
    public boolean getBuiltInZoomControls() {
        return mBvSettings.getBuiltInZoomControls();
    }

    @Override
    public void setDisplayZoomControls(boolean enabled) {
        mBvSettings.setDisplayZoomControls(enabled);
    }

    @Override
    public boolean getDisplayZoomControls() {
        return mBvSettings.getDisplayZoomControls();
    }

    @Override
    public void setAllowFileAccess(boolean allow) {
        mBvSettings.setAllowFileAccess(allow);
    }

    @Override
    public boolean getAllowFileAccess() {
        return mBvSettings.getAllowFileAccess();
    }

    @Override
    public void setAllowContentAccess(boolean allow) {
        mBvSettings.setAllowContentAccess(allow);
    }

    @Override
    public boolean getAllowContentAccess() {
        return mBvSettings.getAllowContentAccess();
    }

    @Override
    public void setLoadWithOverviewMode(boolean overview) {
        mBvSettings.setLoadWithOverviewMode(overview);
    }

    @Override
    public boolean getLoadWithOverviewMode() {
        return mBvSettings.getLoadWithOverviewMode();
    }

    @Override
    public void setAcceptThirdPartyCookies(boolean accept) {
        mBvSettings.setAcceptThirdPartyCookies(accept);
    }

    @Override
    public boolean getAcceptThirdPartyCookies() {
        return mBvSettings.getAcceptThirdPartyCookies();
    }

    @Override
    public void setSaveFormData(boolean save) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) return;

        mBvSettings.setSaveFormData(save);
    }

    @Override
    public boolean getSaveFormData() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) return false;

        return mBvSettings.getSaveFormData();
    }

    @Override
    public synchronized void setTextZoom(int textZoom) {
        mBvSettings.setTextZoom(textZoom);
    }

    @Override
    public synchronized int getTextZoom() {
        return mBvSettings.getTextZoom();
    }

    @Override
    public synchronized void setUserAgent(int ua) {
        mBvSettings.setUserAgent(ua);
    }

    @Override
    public synchronized int getUserAgent() {
        // Minimal implementation for backwards compatibility: just identifies default vs custom.
        return BvSettings.getDefaultUserAgent().equals(getUserAgentString()) ? 0 : -1;
    }

    @Override
    public synchronized void setUseWideViewPort(boolean use) {
        mBvSettings.setUseWideViewPort(use);
    }

    @Override
    public synchronized boolean getUseWideViewPort() {
        return mBvSettings.getUseWideViewPort();
    }

    @Override
    public synchronized void setSupportMultipleWindows(boolean support) {
        mBvSettings.setSupportMultipleWindows(support);
    }

    @Override
    public synchronized boolean supportMultipleWindows() {
        return mBvSettings.supportMultipleWindows();
    }

    @Override
    public synchronized void setLayoutAlgorithm(LayoutAlgorithm l) {
        switch (l) {
            case NORMAL:
                mBvSettings.setLayoutAlgorithm(BvSettings.LAYOUT_ALGORITHM_NORMAL);
                return;
            case SINGLE_COLUMN:
                mBvSettings.setLayoutAlgorithm(BvSettings.LAYOUT_ALGORITHM_SINGLE_COLUMN);
                return;
            case NARROW_COLUMNS:
                mBvSettings.setLayoutAlgorithm(BvSettings.LAYOUT_ALGORITHM_NARROW_COLUMNS);
                return;
            case TEXT_AUTOSIZING:
                mBvSettings.setLayoutAlgorithm(BvSettings.LAYOUT_ALGORITHM_TEXT_AUTOSIZING);
                return;
            default:
                throw new IllegalArgumentException("Unsupported value: " + l);
        }
    }

    @Override
    public synchronized LayoutAlgorithm getLayoutAlgorithm() {
        int value = mBvSettings.getLayoutAlgorithm();
        switch (value) {
            case BvSettings.LAYOUT_ALGORITHM_NORMAL:
                return LayoutAlgorithm.NORMAL;
            case BvSettings.LAYOUT_ALGORITHM_SINGLE_COLUMN:
                return LayoutAlgorithm.SINGLE_COLUMN;
            case BvSettings.LAYOUT_ALGORITHM_NARROW_COLUMNS:
                return LayoutAlgorithm.NARROW_COLUMNS;
            case BvSettings.LAYOUT_ALGORITHM_TEXT_AUTOSIZING:
                return LayoutAlgorithm.TEXT_AUTOSIZING;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }

    @Override
    public synchronized void setStandardFontFamily(String font) {
        mBvSettings.setStandardFontFamily(font);
    }

    @Override
    public synchronized String getStandardFontFamily() {
        return mBvSettings.getStandardFontFamily();
    }

    @Override
    public synchronized void setFixedFontFamily(String font) {
        mBvSettings.setFixedFontFamily(font);
    }

    @Override
    public synchronized String getFixedFontFamily() {
        return mBvSettings.getFixedFontFamily();
    }

    @Override
    public synchronized void setSansSerifFontFamily(String font) {
        mBvSettings.setSansSerifFontFamily(font);
    }

    @Override
    public synchronized String getSansSerifFontFamily() {
        return mBvSettings.getSansSerifFontFamily();
    }

    @Override
    public synchronized void setSerifFontFamily(String font) {
        mBvSettings.setSerifFontFamily(font);
    }

    @Override
    public synchronized String getSerifFontFamily() {
        return mBvSettings.getSerifFontFamily();
    }

    @Override
    public synchronized void setCursiveFontFamily(String font) {
        mBvSettings.setCursiveFontFamily(font);
    }

    @Override
    public synchronized String getCursiveFontFamily() {
        return mBvSettings.getCursiveFontFamily();
    }

    @Override
    public synchronized void setFantasyFontFamily(String font) {
        mBvSettings.setFantasyFontFamily(font);
    }

    @Override
    public synchronized String getFantasyFontFamily() {
        return mBvSettings.getFantasyFontFamily();
    }

    @Override
    public synchronized void setMinimumFontSize(int size) {
        mBvSettings.setMinimumFontSize(size);
    }

    @Override
    public synchronized int getMinimumFontSize() {
        return mBvSettings.getMinimumFontSize();
    }

    @Override
    public synchronized void setMinimumLogicalFontSize(int size) {
        mBvSettings.setMinimumLogicalFontSize(size);
    }

    @Override
    public synchronized int getMinimumLogicalFontSize() {
        return mBvSettings.getMinimumLogicalFontSize();
    }

    @Override
    public synchronized void setDefaultFontSize(int size) {
        mBvSettings.setDefaultFontSize(size);
    }

    @Override
    public synchronized int getDefaultFontSize() {
        return mBvSettings.getDefaultFontSize();
    }

    @Override
    public synchronized void setDefaultFixedFontSize(int size) {
        mBvSettings.setDefaultFixedFontSize(size);
    }

    @Override
    public synchronized int getDefaultFixedFontSize() {
        return mBvSettings.getDefaultFixedFontSize();
    }

    @Override
    public synchronized void setLoadsImagesAutomatically(boolean flag) {
        mBvSettings.setLoadsImagesAutomatically(flag);
    }

    @Override
    public synchronized boolean getLoadsImagesAutomatically() {
        return mBvSettings.getLoadsImagesAutomatically();
    }

    @Override
    public synchronized void setBlockNetworkImage(boolean flag) {
        mBvSettings.setImagesEnabled(!flag);
    }

    @Override
    public synchronized boolean getBlockNetworkImage() {
        return !mBvSettings.getImagesEnabled();
    }

    @Override
    public synchronized void setBlockNetworkLoads(boolean flag) {
        mBvSettings.setBlockNetworkLoads(flag);
    }

    @Override
    public synchronized boolean getBlockNetworkLoads() {
        return mBvSettings.getBlockNetworkLoads();
    }

    @Override
    public synchronized void setJavaScriptEnabled(boolean flag) {
        mBvSettings.setJavaScriptEnabled(flag);
    }

    @Override
    public void setAllowUniversalAccessFromFileURLs(boolean flag) {
        mBvSettings.setAllowUniversalAccessFromFileURLs(flag);
    }

    @Override
    public void setAllowFileAccessFromFileURLs(boolean flag) {
        mBvSettings.setAllowFileAccessFromFileURLs(flag);
    }

    @Override
    public synchronized void setPluginsEnabled(boolean flag) {
        mPluginState = flag ? PluginState.ON : PluginState.OFF;
    }

    @Override
    public synchronized void setPluginState(PluginState state) {
        mPluginState = state;
    }

    @Override
    public synchronized void setAppCacheEnabled(boolean flag) {
        mBvSettings.setAppCacheEnabled(flag);
    }

    @Override
    public synchronized void setAppCachePath(String appCachePath) {
        mBvSettings.setAppCachePath(appCachePath);
    }

    @Override
    public synchronized void setDatabaseEnabled(boolean flag) {
        mBvSettings.setDatabaseEnabled(flag);
    }

    @Override
    public synchronized void setDomStorageEnabled(boolean flag) {
        mBvSettings.setDomStorageEnabled(flag);
    }

    @Override
    public synchronized boolean getDomStorageEnabled() {
        return mBvSettings.getDomStorageEnabled();
    }

    @Override
    public synchronized boolean getDatabaseEnabled() {
        return mBvSettings.getDatabaseEnabled();
    }

    @Override
    public synchronized void setGeolocationEnabled(boolean flag) {
        mBvSettings.setGeolocationEnabled(flag);
    }

    @Override
    public synchronized boolean getJavaScriptEnabled() {
        return mBvSettings.getJavaScriptEnabled();
    }

    @Override
    public boolean getAllowUniversalAccessFromFileURLs() {
        return mBvSettings.getAllowUniversalAccessFromFileURLs();
    }

    @Override
    public boolean getAllowFileAccessFromFileURLs() {
        return mBvSettings.getAllowFileAccessFromFileURLs();
    }

    @Override
    public synchronized boolean getPluginsEnabled() {
        return mPluginState == PluginState.ON;
    }

    @Override
    public synchronized PluginState getPluginState() {
        return mPluginState;
    }

    @Override
    public synchronized void setJavaScriptCanOpenWindowsAutomatically(boolean flag) {
        mBvSettings.setJavaScriptCanOpenWindowsAutomatically(flag);
    }

    @Override
    public synchronized boolean getJavaScriptCanOpenWindowsAutomatically() {
        return mBvSettings.getJavaScriptCanOpenWindowsAutomatically();
    }

    @Override
    public synchronized void setDefaultTextEncodingName(String encoding) {
        mBvSettings.setDefaultTextEncodingName(encoding);
    }

    @Override
    public synchronized String getDefaultTextEncodingName() {
        return mBvSettings.getDefaultTextEncodingName();
    }

    @Override
    public synchronized void setUserAgentString(String ua) {
        mBvSettings.setUserAgentString(ua);
    }

    @Override
    public synchronized String getUserAgentString() {
        return mBvSettings.getUserAgentString();
    }

    @Override
    public void setNeedInitialFocus(boolean flag) {
        mBvSettings.setShouldFocusFirstNode(flag);
    }

    @Override
    public void setCacheMode(int mode) {
        mBvSettings.setCacheMode(mode);
    }

    @Override
    public int getCacheMode() {
        return mBvSettings.getCacheMode();
    }

    @Override
    public void setMediaPlaybackRequiresUserGesture(boolean require) {
        mBvSettings.setMediaPlaybackRequiresUserGesture(require);
    }

    @Override
    public boolean getMediaPlaybackRequiresUserGesture() {
        return mBvSettings.getMediaPlaybackRequiresUserGesture();
    }

    @Override
    public void setMixedContentMode(int mode) {
        mBvSettings.setMixedContentMode(mode);
    }

    @Override
    public int getMixedContentMode() {
        return mBvSettings.getMixedContentMode();
    }

    @Override
    public void setOffscreenPreRaster(boolean enabled) {
        mBvSettings.setOffscreenPreRaster(enabled);
    }

    @Override
    public boolean getOffscreenPreRaster() {
        return mBvSettings.getOffscreenPreRaster();
    }

    @Override
    public void setDisabledActionModeMenuItems(int menuItems) {
        mBvSettings.setDisabledActionModeMenuItems(menuItems);
    }

    @Override
    public int getDisabledActionModeMenuItems() {
        return mBvSettings.getDisabledActionModeMenuItems();
    }

    @Override
    public void setVideoOverlayForEmbeddedEncryptedVideoEnabled(boolean flag) {
        // No-op, see http://crbug.com/616583
    }

    @Override
    public boolean getVideoOverlayForEmbeddedEncryptedVideoEnabled() {
        // Always false, see http://crbug.com/616583
        return false;
    }

    @Override
    public void setForceDark(int forceDarkMode) {
        // jiang
        // switch (forceDarkMode) {
        //     case BisonViewSettings.FORCE_DARK_OFF:
        //         getBvSettings().setForceDarkMode(BvSettings.FORCE_DARK_OFF);
        //         break;
        //     case BisonViewSettings.FORCE_DARK_AUTO:
        //         getBvSettings().setForceDarkMode(BvSettings.FORCE_DARK_AUTO);
        //         break;
        //     case BisonViewSettings.FORCE_DARK_ON:
        //         getBvSettings().setForceDarkMode(BvSettings.FORCE_DARK_ON);
        //         break;
        //     default:
        //         throw new IllegalArgumentException(
        //                 "Force dark mode is not one of FORCE_DARK_(ON|OFF|AUTO)");
        // }
    }

    @Override
    public int getForceDark() {
        // jiang
        // switch (getBvSettings().getForceDarkMode()) {
        //     case BvSettings.FORCE_DARK_OFF:
        //         return BisonViewSettings.FORCE_DARK_OFF;
        //     case BvSettings.FORCE_DARK_AUTO:
        //         return BisonViewSettings.FORCE_DARK_AUTO;
        //     case BvSettings.FORCE_DARK_ON:
        //         return BisonViewSettings.FORCE_DARK_ON;
        // }
        return BisonViewSettings.FORCE_DARK_AUTO;
    }




}
