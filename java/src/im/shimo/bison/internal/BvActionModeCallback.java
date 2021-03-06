// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.internal;

import org.chromium.base.PackageManagerUtils;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.content.R;
import org.chromium.content_public.browser.ActionModeCallbackHelper;
import org.chromium.content_public.browser.SelectionPopupController;
import org.chromium.content_public.browser.WebContents;

import android.app.SearchManager;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.text.TextUtils;
import android.view.ActionMode;
import android.view.Menu;
import android.view.MenuItem;
import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public class BvActionModeCallback implements ActionMode.Callback {
    private final BvContents mBisonContents;
    private final ActionModeCallbackHelper mHelper;
    private int mAllowedMenuItems;

    public BvActionModeCallback(Context context, BvContents bvContents, WebContents webContents) {
        mBisonContents = bvContents;
        mHelper =
                SelectionPopupController.fromWebContents(webContents).getActionModeCallbackHelper();
        mHelper.setAllowedMenuItems(0);  // No item is allowed by default for WebView.
    }

    @Override
    public boolean onCreateActionMode(ActionMode mode, Menu menu) {
        int allowedItems = (getAllowedMenu(ActionModeCallbackHelper.MENU_ITEM_SHARE)
                | getAllowedMenu(ActionModeCallbackHelper.MENU_ITEM_WEB_SEARCH)
                | getAllowedMenu(ActionModeCallbackHelper.MENU_ITEM_PROCESS_TEXT));
        if (allowedItems != mAllowedMenuItems) {
            mHelper.setAllowedMenuItems(allowedItems);
            mAllowedMenuItems = allowedItems;
        }
        mHelper.onCreateActionMode(mode, menu);
        return true;
    }

    private int getAllowedMenu(int menuItem) {
        boolean showItem = true;
        if (menuItem == ActionModeCallbackHelper.MENU_ITEM_WEB_SEARCH) {
            showItem = isWebSearchAvailable();
        }
        return showItem && mBisonContents.isSelectActionModeAllowed(menuItem) ? menuItem : 0;
    }

    private boolean isWebSearchAvailable() {
        Intent intent = new Intent(Intent.ACTION_WEB_SEARCH);
        intent.putExtra(SearchManager.EXTRA_NEW_SEARCH, true);
        return !PackageManagerUtils.queryIntentActivities(intent, PackageManager.MATCH_DEFAULT_ONLY)
                        .isEmpty();
    }

    @Override
    public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
        return mHelper.onPrepareActionMode(mode, menu);
    }

    @Override
    public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
        if (!mHelper.isActionModeValid()) return true;

        int groupId = item.getGroupId();

        if (groupId == R.id.select_action_menu_text_processing_menus) {
            processText(item.getIntent());
            // The ActionMode is not dismissed to match the behavior with
            // TextView in Android M.
        } else {
            return mHelper.onActionItemClicked(mode, item);
        }
        return true;
    }

    @Override
    public void onDestroyActionMode(ActionMode mode) {
        mHelper.onDestroyActionMode();
    }

    private void processText(Intent intent) {
        RecordUserAction.record("MobileActionMode.ProcessTextIntent");

        String query = ActionModeCallbackHelper.sanitizeQuery(mHelper.getSelectedText(),
                ActionModeCallbackHelper.MAX_SEARCH_QUERY_LENGTH);
        if (TextUtils.isEmpty(query)) return;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            intent.putExtra(Intent.EXTRA_PROCESS_TEXT, query);
        }
        try {
            mBisonContents.startProcessTextIntent(intent);
        } catch (ActivityNotFoundException ex) {
            // If no app handles it, do nothing.
        }
    }
}
