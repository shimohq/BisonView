package im.shimo.bison;

import androidx.annotation.Nullable;

import java.security.Principal;
import java.security.PrivateKey;
import java.security.cert.X509Certificate;


public abstract class ClientCertRequest {

    public ClientCertRequest() { }

    /**
     * Returns the acceptable types of asymmetric keys.
     */
    @Nullable
    public abstract String[] getKeyTypes();

    /**
     * Returns the acceptable certificate issuers for the certificate
     *            matching the private key.
     */
    @Nullable
    public abstract Principal[] getPrincipals();

    /**
     * Returns the host name of the server requesting the certificate.
     */
    public abstract String getHost();

    /**
     * Returns the port number of the server requesting the certificate.
     */
    public abstract int getPort();

    /**
     * Proceed with the specified private key and client certificate chain.
     * Remember the user's positive choice and use it for future requests.
     */
    public abstract void proceed(PrivateKey privateKey, X509Certificate[] chain);

    /**
     * Ignore the request for now. Do not remember user's choice.
     */
    public abstract void ignore();

    /**
     * Cancel this request. Remember the user's choice and use it for
     * future requests.
     */
    public abstract void cancel();
}
