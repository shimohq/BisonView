package im.shimo.bison;

import static androidx.annotation.RestrictTo.Scope.LIBRARY_GROUP;

import androidx.annotation.RestrictTo;

public abstract class WebResourceError {

    /**
     * Gets the error code of the error. The code corresponds to one
     * of the ERROR_* constants in {@link BisonViewClient}.
     *
     * @return The error code of the error
     */
    public abstract int getErrorCode();

    /**
     * Gets the string describing the error. Descriptions are localized,
     * and thus can be used for communicating the problem to the user.
     *
     * @return The description of the error
     */
    public abstract CharSequence getDescription();

    /**
     * This class can not be subclassed by applications.
     */
    @RestrictTo(LIBRARY_GROUP)
    public WebResourceError() {}

}
