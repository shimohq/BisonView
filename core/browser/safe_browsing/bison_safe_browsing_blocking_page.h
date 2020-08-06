// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_BROWSER_SAFE_BROWSING_BISON_SAFE_BROWSING_BLOCKING_PAGE_H_
#define BISON_BROWSER_SAFE_BROWSING_BISON_SAFE_BROWSING_BLOCKING_PAGE_H_

#include "components/safe_browsing/base_blocking_page.h"
#include "components/security_interstitials/core/base_safe_browsing_error_ui.h"

namespace security_interstitials {
struct UnsafeResource;
}  // namespace security_interstitials

namespace bison {

class BisonSafeBrowsingUIManager;

class BisonSafeBrowsingBlockingPage : public safe_browsing::BaseBlockingPage {
 public:
  typedef security_interstitials::UnsafeResource UnsafeResource;

  static void ShowBlockingPage(BisonSafeBrowsingUIManager* ui_manager,
                               const UnsafeResource& unsafe_resource);

 protected:
  // Used to specify which BaseSafeBrowsingErrorUI to instantiate, and
  // parameters they require.
  // Note: these values are persisted in UMA logs, so they should never be
  // renumbered nor reused.
  // TODO jiang java package
  // GENERATED_JAVA_ENUM_PACKAGE: im.shimo.bison.core
  enum class ErrorUiType { LOUD, QUIET_SMALL, QUIET_GIANT, COUNT };

  // Don't instantiate this class directly, use ShowBlockingPage instead.
  BisonSafeBrowsingBlockingPage(
      BisonSafeBrowsingUIManager* ui_manager,
      content::WebContents* web_contents,
      const GURL& main_frame_url,
      const UnsafeResourceList& unsafe_resources,
      std::unique_ptr<
          security_interstitials::SecurityInterstitialControllerClient>
          controller_client,
      const BaseSafeBrowsingErrorUI::SBErrorDisplayOptions& display_options,
      ErrorUiType errorUiType);

  // Called when the interstitial is going away. If there is a
  // pending threat details object, we look at the user's
  // preferences, and if the option to send threat details is
  // enabled, the report is scheduled to be sent on the |ui_manager_|.
  void FinishThreatDetails(const base::TimeDelta& delay,
                           bool did_proceed,
                           int num_visits) override;

  // Whether ThreatDetails collection is in progress as part of this
  // interstitial.
  bool threat_details_in_progress_;
};

}  // namespace bison

#endif  // BISON_BROWSER_SAFE_BROWSING_BISON_SAFE_BROWSING_BLOCKING_PAGE_H_