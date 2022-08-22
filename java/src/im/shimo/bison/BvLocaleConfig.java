// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison;

/**
 * Simple class that provides access to the array of uncompressed pak locales. See
 * //bison/BUILD.gn for more details.
 */
public final class BvLocaleConfig {
    private BvLocaleConfig() {}

    public static String[] getWebViewSupportedPakLocales() {
        return ProductConfig.LOCALES;
    }
}
