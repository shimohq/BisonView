package im.shimo.bison.internal;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

/**
 * BisonVariationsSeedBridge stores the seed loaded in Java for use in native. Native should clear the
 * stored seed after retrieving it, since the seed isn't needed in Java.
 */
public final class BvVariationsSeedBridge {
    private static SeedInfo sSeed;
    private static boolean sIsLoadedSeedFresh;

    public static void setSeed(SeedInfo seed) {
        assert sSeed == null; // The seed should only be loaded once.
        sSeed = seed;
    }

    public static void setLoadedSeedFresh(boolean isLoadedSeedFresh) {
        sIsLoadedSeedFresh = isLoadedSeedFresh;
    }

    @CalledByNative
    private static boolean haveSeed() {
        return sSeed != null;
    }

    @CalledByNative
    private static void clearSeed() {
        sSeed = null; // Allow GC.
    }

    @CalledByNative
    private static String getSignature() {
        return sSeed.signature;
    }

    @CalledByNative
    private static String getCountry() {
        return sSeed.country;
    }

    @CalledByNative
    private static long getDate() {
        return sSeed.date;
    }

    @CalledByNative
    private static boolean getIsGzipCompressed() {
        return sSeed.isGzipCompressed;
    }

    @CalledByNative
    private static byte[] getData() {
        return sSeed.seedData;
    }

    @CalledByNative
    private static boolean isLoadedSeedFresh() {
        return sIsLoadedSeedFresh;
    }

    private BvVariationsSeedBridge() {}
}
