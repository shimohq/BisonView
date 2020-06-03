// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.core;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

/**
 * Exposes a subset of Chromium form database to Webview database for managing autocomplete
 * functionality.
 */
@JNINamespace("bison")
public class BisonFormDatabase {

    public static boolean hasFormData() {
        return BisonFormDatabaseJni.get().hasFormData();
    }

    public static void clearFormData() {
        BisonFormDatabaseJni.get().clearFormData();
    }

    @NativeMethods
    interface Natives {
        boolean hasFormData();
        void clearFormData();
    }
}
