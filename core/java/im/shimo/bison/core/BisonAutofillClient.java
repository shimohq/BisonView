// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.core;

import android.content.Context;
import android.view.View;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.components.autofill.AutofillDelegate;
import org.chromium.components.autofill.AutofillPopup;
import org.chromium.components.autofill.AutofillSuggestion;
import org.chromium.ui.DropdownItem;

/**
 * Java counterpart to the AwAutofillClient. This class is owned by AwContents and has
 * a weak reference from native side.
 */
@JNINamespace("bison")
public class BisonAutofillClient {

    private final long mNativeBisonAutofillClient;
    private AutofillPopup mAutofillPopup;
    private Context mContext;

    @CalledByNative
    public static BisonAutofillClient create(long nativeClient) {
        return new BisonAutofillClient(nativeClient);
    }

    private BisonAutofillClient(long nativeBisonAutofillClient) {
        mNativeBisonAutofillClient = nativeBisonAutofillClient;
    }

    public void init(Context context) {
        mContext = context;
    }

    @CalledByNative
    private void showAutofillPopup(View anchorView, boolean isRtl,
            AutofillSuggestion[] suggestions) {

        if (mAutofillPopup == null) {
            if (ContextUtils.activityFromContext(mContext) == null) {
                BisonAutofillClientJni.get().dismissed(mNativeBisonAutofillClient, BisonAutofillClient.this);
                return;
            }
            try {
                mAutofillPopup = new AutofillPopup(mContext, anchorView, new AutofillDelegate() {
                    @Override
                    public void dismissed() {
                        BisonAutofillClientJni.get().dismissed(
                                mNativeBisonAutofillClient, BisonAutofillClient.this);
                    }
                    @Override
                    public void suggestionSelected(int listIndex) {
                        BisonAutofillClientJni.get().suggestionSelected(
                                mNativeBisonAutofillClient, BisonAutofillClient.this, listIndex);
                    }
                    @Override
                    public void deleteSuggestion(int listIndex) {}

                    @Override
                    public void accessibilityFocusCleared() {}
                });
            } catch (RuntimeException e) {
                // Deliberately swallowing exception because bad fraemwork implementation can
                // throw exceptions in ListPopupWindow constructor.
                BisonAutofillClientJni.get().dismissed(mNativeBisonAutofillClient, BisonAutofillClient.this);
                return;
            }
        }
        mAutofillPopup.filterAndShow(suggestions, isRtl, false);
    }

    @CalledByNative
    public void hideAutofillPopup() {
        if (mAutofillPopup == null) return;
        mAutofillPopup.dismiss();
        mAutofillPopup = null;
    }

    @CalledByNative
    private static AutofillSuggestion[] createAutofillSuggestionArray(int size) {
        return new AutofillSuggestion[size];
    }

    /**
     * @param array AutofillSuggestion array that should get a new suggestion added.
     * @param index Index in the array where to place a new suggestion.
     * @param name Name of the suggestion.
     * @param label Label of the suggestion.
     * @param uniqueId Unique suggestion id.
     */
    @CalledByNative
    private static void addToAutofillSuggestionArray(AutofillSuggestion[] array, int index,
            String name, String label, int uniqueId) {
        array[index] = new AutofillSuggestion(name, label, DropdownItem.NO_ICON,
                false /* isIconAtLeft */, uniqueId, false /* isDeletable */,
                false /* isMultilineLabel */, false /* isBoldLabel */);
    }

    @NativeMethods
    interface Natives {
        void dismissed(long nativeBisonAutofillClient, BisonAutofillClient caller);
        void suggestionSelected(long nativeBisonAutofillClient, BisonAutofillClient caller, int position);
    }
}
