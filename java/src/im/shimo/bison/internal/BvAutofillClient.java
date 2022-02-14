package im.shimo.bison.internal;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.components.autofill.AutofillDelegate;
import org.chromium.components.autofill.AutofillPopup;
import org.chromium.components.autofill.AutofillSuggestion;
import org.chromium.ui.DropdownItem;

import android.content.Context;
import android.view.View;
import androidx.annotation.RestrictTo;
import im.shimo.bison.internal.BvAutofillClient;

/**
 * Java counterpart to the BvAutofillClient. This class is owned by BvContents and has
 * a weak reference from native side.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
@JNINamespace("bison")
public class BvAutofillClient {

    private final long mNativeBvAutofillClient;
    private AutofillPopup mAutofillPopup;
    private Context mContext;

    @CalledByNative
    public static BvAutofillClient create(long nativeClient) {
        return new BvAutofillClient(nativeClient);
    }

    private BvAutofillClient(long nativeBvAutofillClient) {
        mNativeBvAutofillClient = nativeBvAutofillClient;
    }

    public void init(Context context) {
        mContext = context;
    }

    @CalledByNative
    private void showAutofillPopup(View anchorView, boolean isRtl,
            AutofillSuggestion[] suggestions) {

        if (mAutofillPopup == null) {
            if (ContextUtils.activityFromContext(mContext) == null) {
                BvAutofillClientJni.get().dismissed(mNativeBvAutofillClient, BvAutofillClient.this);
                return;
            }
            try {
                mAutofillPopup = new AutofillPopup(mContext, anchorView, new AutofillDelegate() {
                    @Override
                    public void dismissed() {
                        BvAutofillClientJni.get().dismissed(
                                mNativeBvAutofillClient, BvAutofillClient.this);
                    }
                    @Override
                    public void suggestionSelected(int listIndex) {
                        BvAutofillClientJni.get().suggestionSelected(
                                mNativeBvAutofillClient, BvAutofillClient.this, listIndex);
                    }
                    @Override
                    public void deleteSuggestion(int listIndex) {}

                    @Override
                    public void accessibilityFocusCleared() {}
                });
            } catch (RuntimeException e) {
                // Deliberately swallowing exception because bad fraemwork implementation can
                // throw exceptions in ListPopupWindow constructor.
                BvAutofillClientJni.get().dismissed(mNativeBvAutofillClient, BvAutofillClient.this);
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
        void dismissed(long nativeBvAutofillClient, BvAutofillClient caller);
        void suggestionSelected(long nativeBvAutofillClient, BvAutofillClient caller, int position);
    }
}
