// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.adapter;

import static android.util.Patterns.GOOD_IRI_CHAR;

import androidx.annotation.NonNull;

import org.chromium.base.Log;

import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import im.shimo.bison.BisonView;
import im.shimo.bison.CookieManager;
import im.shimo.bison.ValueCallback;
import im.shimo.bison.internal.BvCookieManager;


@SuppressWarnings({"deprecation", "NoSynchronizedMethodCheck"})
public class CookieManagerAdapter extends CookieManager {
    private static final String TAG = "CookieManager";

    BvCookieManager mCookieManager;

    public CookieManagerAdapter(BvCookieManager cookieManager) {
        mCookieManager = cookieManager;
    }

    @Override
    public synchronized void setAcceptCookie(boolean accept) {
        mCookieManager.setAcceptCookie(accept);
    }

    @Override
    public synchronized boolean acceptCookie() {
        return mCookieManager.acceptCookie();
    }

    @Override
    public synchronized void setAcceptThirdPartyCookies(BisonView bisonView, boolean accept) {
        bisonView.getSettings().setAcceptThirdPartyCookies(accept);
    }

    @Override
    public synchronized boolean acceptThirdPartyCookies(BisonView bisonView) {
        return bisonView.getSettings().getAcceptThirdPartyCookies();
    }

    @Override
    public void setCookie(String url, String value) {
        if (value == null) {
            Log.e(TAG, "Not setting cookie with null value for URL: %s", url);
            return;
        }

        try {
            mCookieManager.setCookie(fixupUrl(url), value);
        } catch (RuntimeException e) {
            Log.e(TAG, "Not setting cookie due to error parsing URL: %s", url, e);
        }
    }

    @Override
    public void setCookie(String url, String value, ValueCallback<Boolean> callback) {
        if (value == null) {
            Log.e(TAG, "Not setting cookie with null value for URL: %s", url);
            return;
        }

        try {
            mCookieManager.setCookie(
                    fixupUrl(url), value, CallbackConverter.fromValueCallback(callback));
        } catch (RuntimeException e) {
            Log.e(TAG, "Not setting cookie due to error parsing URL: %s", url, e);
        }
    }

    @Override
    public String getCookie(String url) {
        try {
            return mCookieManager.getCookie(fixupUrl(url));
        } catch (RuntimeException e) {
            Log.e(TAG, "Unable to get cookies due to error parsing URL: %s", url, e);
            return null;
        }
    }


    @Override
    public void removeSessionCookie() {
        mCookieManager.removeSessionCookies();
    }

    @Override
    public void removeSessionCookies(final ValueCallback<Boolean> callback) {
        mCookieManager.removeSessionCookies(CallbackConverter.fromValueCallback(callback));
    }

    @Override
    public void removeAllCookie() {
        mCookieManager.removeAllCookies();
    }

    @Override
    public void removeAllCookies(final ValueCallback<Boolean> callback) {
        mCookieManager.removeAllCookies(CallbackConverter.fromValueCallback(callback));
    }

    @Override
    public synchronized boolean hasCookies() {
        return mCookieManager.hasCookies();
    }

    @Override
    public synchronized boolean hasCookies(boolean privateBrowsing) {
        return mCookieManager.hasCookies();
    }

    @Override
    public void removeExpiredCookie() {
        mCookieManager.removeExpiredCookies();
    }

    @Override
    public void flush() {
        mCookieManager.flushCookieStore();
    }

    @Override
    protected boolean allowFileSchemeCookiesImpl() {
        return mCookieManager.allowFileSchemeCookies();
    }

    @Override
    protected void setAcceptFileSchemeCookiesImpl(boolean accept) {
        mCookieManager.setAcceptFileSchemeCookies(accept);
    }




    private static String fixupUrl(String url) throws RuntimeException {
        return new WebAddress(url).toString();
    }

    private static class WebAddress {

        static final int MATCH_GROUP_SCHEME = 1;
        static final int MATCH_GROUP_AUTHORITY = 2;
        static final int MATCH_GROUP_HOST = 3;
        static final int MATCH_GROUP_PORT = 4;
        static final int MATCH_GROUP_PATH = 5;

        private String mScheme;
        private String mHost;
        private int mPort;
        private String mPath;
        private String mAuthInfo;

        static Pattern sAddressPattern = Pattern.compile(
                /* scheme    */ "(?:(http|https|file)\\:\\/\\/)?" +
                /* authority */ "(?:([-A-Za-z0-9$_.+!*'(),;?&=]+(?:\\:[-A-Za-z0-9$_.+!*'(),;?&=]+)?)@)?" +
                /* host      */ "([" + GOOD_IRI_CHAR + "%_-][" + GOOD_IRI_CHAR + "%_\\.-]*|\\[[0-9a-fA-F:\\.]+\\])?" +
                /* port      */ "(?:\\:([0-9]*))?" +
                /* path      */ "(\\/?[^#]*)?" +
                /* anchor    */ ".*", Pattern.CASE_INSENSITIVE);

        WebAddress(String address) throws RuntimeException {
            if (address == null) {
                throw new NullPointerException();
            }

            mScheme = "";
            mHost = "";
            mPort = -1;
            mPath = "/";
            mAuthInfo = "";

            Matcher m = sAddressPattern.matcher(address);
            String t;
            if (m.matches()) {
                t = m.group(MATCH_GROUP_SCHEME);
                if (t != null) mScheme = t.toLowerCase(Locale.ROOT);
                t = m.group(MATCH_GROUP_AUTHORITY);
                if (t != null) mAuthInfo = t;
                t = m.group(MATCH_GROUP_HOST);
                if (t != null) mHost = t;
                t = m.group(MATCH_GROUP_PORT);
                if (t != null && t.length() > 0) {
                    // The ':' character is not returned by the regex.
                    try {
                        mPort = Integer.parseInt(t);
                    } catch (NumberFormatException ex) {
                        throw new RuntimeException("Bad port");
                    }
                }
                t = m.group(MATCH_GROUP_PATH);
                if (t != null && t.length() > 0) {
                /* handle busted myspace frontpage redirect with
                   missing initial "/" */
                    if (t.charAt(0) == '/') {
                        mPath = t;
                    } else {
                        mPath = "/" + t;
                    }
                }

            } else {
                throw new RuntimeException("Bad address");
            }

            if (mPort == 443 && mScheme.equals("")) {
                mScheme = "https";
            } else if (mPort == -1) {
                if (mScheme.equals("https"))
                    mPort = 443;
                else
                    mPort = 80; // default
            }
            if (mScheme.equals("")) mScheme = "http";
        }

        @NonNull
        @Override
        public String toString() {
            String port = "";
            if ((mPort != 443 && mScheme.equals("https")) ||
                    (mPort != 80 && mScheme.equals("http"))) {
                port = ":" + Integer.toString(mPort);
            }
            String authInfo = "";
            if (mAuthInfo.length() > 0) {
                authInfo = mAuthInfo + "@";
            }

            return mScheme + "://" + authInfo + mHost + port + mPath;
        }

    }


}
