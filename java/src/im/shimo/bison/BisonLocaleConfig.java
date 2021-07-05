package im.shimo.bison;

final class BisonLocaleConfig {
    
    private BisonLocaleConfig() {}

    public static String[] getWebViewSupportedPakLocales() {
        return ProductConfig.UNCOMPRESSED_LOCALES;
    }
}
