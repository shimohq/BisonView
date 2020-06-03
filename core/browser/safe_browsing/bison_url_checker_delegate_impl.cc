// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/core/browser/safe_browsing/bison_url_checker_delegate_impl.h"

#include "bison/core/browser/bison_browser_context.h"
#include "bison/core/browser/bison_contents_client_bridge.h"
#include "bison/core/browser/bison_contents_io_thread_client.h"
#include "bison/core/browser/network_service/bison_web_resource_request.h"
#include "bison/core/browser/safe_browsing/bison_safe_browsing_ui_manager.h"
#include "bison/core/browser/safe_browsing/bison_safe_browsing_whitelist_manager.h"
#include "base/bind.h"
#include "base/task/post_task.h"
#include "components/safe_browsing/db/database_manager.h"
#include "components/safe_browsing/db/v4_protocol_manager_util.h"
#include "components/security_interstitials/content/unsafe_resource.h"
#include "components/security_interstitials/core/urls.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"

namespace bison {

BisonUrlCheckerDelegateImpl::BisonUrlCheckerDelegateImpl(
    scoped_refptr<safe_browsing::SafeBrowsingDatabaseManager> database_manager,
    scoped_refptr<BisonSafeBrowsingUIManager> ui_manager,
    BisonSafeBrowsingWhitelistManager* whitelist_manager)
    : database_manager_(std::move(database_manager)),
      ui_manager_(std::move(ui_manager)),
      threat_types_(safe_browsing::CreateSBThreatTypeSet(
          {safe_browsing::SB_THREAT_TYPE_URL_MALWARE,
           safe_browsing::SB_THREAT_TYPE_URL_PHISHING,
           safe_browsing::SB_THREAT_TYPE_URL_UNWANTED,
           safe_browsing::SB_THREAT_TYPE_BILLING})),
      whitelist_manager_(whitelist_manager) {}

BisonUrlCheckerDelegateImpl::~BisonUrlCheckerDelegateImpl() = default;

void BisonUrlCheckerDelegateImpl::MaybeDestroyPrerenderContents(
    const base::Callback<content::WebContents*()>& web_contents_getter) {}

void BisonUrlCheckerDelegateImpl::StartDisplayingBlockingPageHelper(
    const security_interstitials::UnsafeResource& resource,
    const std::string& method,
    const net::HttpRequestHeaders& headers,
    bool is_main_frame,
    bool has_user_gesture) {
  BisonWebResourceRequest request(resource.url.spec(), method, is_main_frame,
                               has_user_gesture, headers);

  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&BisonUrlCheckerDelegateImpl::StartApplicationResponse,
                     ui_manager_, resource, std::move(request)));
}

bool BisonUrlCheckerDelegateImpl::IsUrlWhitelisted(const GURL& url) {
  return whitelist_manager_->IsURLWhitelisted(url);
}

bool BisonUrlCheckerDelegateImpl::ShouldSkipRequestCheck(
    content::ResourceContext* resource_context,
    const GURL& original_url,
    int frame_tree_node_id,
    int render_process_id,
    int render_frame_id,
    bool originated_from_service_worker) {
  std::unique_ptr<BisonContentsIoThreadClient> client;

  if (originated_from_service_worker)
    client = BisonContentsIoThreadClient::GetServiceWorkerIoThreadClient();
  else if (render_process_id == -1 || render_frame_id == -1) {
    client = BisonContentsIoThreadClient::FromID(frame_tree_node_id);
  } else {
    client =
        BisonContentsIoThreadClient::FromID(render_process_id, render_frame_id);
  }

  // Consider the request as whitelisted, if SafeBrowsing is not enabled.
  return client && !client->GetSafeBrowsingEnabled();
}

void BisonUrlCheckerDelegateImpl::NotifySuspiciousSiteDetected(
    const base::RepeatingCallback<content::WebContents*()>&
        web_contents_getter) {}

const safe_browsing::SBThreatTypeSet&
BisonUrlCheckerDelegateImpl::GetThreatTypes() {
  return threat_types_;
}

safe_browsing::SafeBrowsingDatabaseManager*
BisonUrlCheckerDelegateImpl::GetDatabaseManager() {
  return database_manager_.get();
}

safe_browsing::BaseUIManager* BisonUrlCheckerDelegateImpl::GetUIManager() {
  return ui_manager_.get();
}

// static
void BisonUrlCheckerDelegateImpl::StartApplicationResponse(
    scoped_refptr<BisonSafeBrowsingUIManager> ui_manager,
    const security_interstitials::UnsafeResource& resource,
    const BisonWebResourceRequest& request) {
  content::WebContents* web_contents = resource.web_contents_getter.Run();
  BisonContentsClientBridge* client =
      BisonContentsClientBridge::FromWebContents(web_contents);

  if (client) {
    base::OnceCallback<void(SafeBrowsingAction, bool)> callback =
        base::BindOnce(&BisonUrlCheckerDelegateImpl::DoApplicationResponse,
                       ui_manager, resource);

    client->OnSafeBrowsingHit(request, resource.threat_type,
                              std::move(callback));
  }
}

// static
void BisonUrlCheckerDelegateImpl::DoApplicationResponse(
    scoped_refptr<BisonSafeBrowsingUIManager> ui_manager,
    const security_interstitials::UnsafeResource& resource,
    SafeBrowsingAction action,
    bool reporting) {
  content::WebContents* web_contents = resource.web_contents_getter.Run();

  if (!reporting) {
    BisonBrowserContext* browser_context =
        BisonBrowserContext::FromWebContents(web_contents);
    browser_context->SetExtendedReportingAllowed(false);
  }

  // TODO(ntfschr): fully handle reporting once we add support (crbug/688629)
  bool proceed;
  switch (action) {
    case SafeBrowsingAction::SHOW_INTERSTITIAL:
      base::PostTask(
          FROM_HERE, {content::BrowserThread::UI},
          base::BindOnce(
              &BisonUrlCheckerDelegateImpl::StartDisplayingDefaultBlockingPage,
              ui_manager, resource));
      return;
    case SafeBrowsingAction::PROCEED:
      proceed = true;
      break;
    case SafeBrowsingAction::BACK_TO_SAFETY:
      proceed = false;
      break;
    default:
      NOTREACHED();
  }

  content::NavigationEntry* entry = resource.GetNavigationEntryForResource();
  GURL main_frame_url = entry ? entry->GetURL() : GURL();

  // Navigate back for back-to-safety on subresources
  if (!proceed && resource.is_subframe) {
    if (web_contents->GetController().CanGoBack()) {
      web_contents->GetController().GoBack();
    } else {
      web_contents->GetController().LoadURL(
          ui_manager->default_safe_page(), content::Referrer(),
          ui::PAGE_TRANSITION_AUTO_TOPLEVEL, std::string());
    }
  }

  ui_manager->OnBlockingPageDone(
      std::vector<security_interstitials::UnsafeResource>{resource}, proceed,
      web_contents, main_frame_url);
}

// static
void BisonUrlCheckerDelegateImpl::StartDisplayingDefaultBlockingPage(
    scoped_refptr<BisonSafeBrowsingUIManager> ui_manager,
    const security_interstitials::UnsafeResource& resource) {
  content::WebContents* web_contents = resource.web_contents_getter.Run();
  if (web_contents) {
    ui_manager->DisplayBlockingPage(resource);
    return;
  }

  // Reporting back that it is not okay to proceed with loading the URL.
  base::PostTask(FROM_HERE, {content::BrowserThread::IO},
                 base::BindOnce(resource.callback, false));
}

}  // namespace bison
