// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison_content_renderer_client.h"

#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "bison_render_view_observer.h"
#include "components/cdm/renderer/external_clear_key_key_system_properties.h"
#include "components/web_cache/renderer/web_cache_impl.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/simple_connection_filter.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "net/base/net_errors.h"
#include "ppapi/buildflags/buildflags.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/blink/public/platform/web_url_error.h"
#include "third_party/blink/public/web/web_view.h"
#include "v8/include/v8.h"

#if BUILDFLAG(ENABLE_MOJO_CDM)  // true
#include "base/feature_list.h"
#include "media/base/media_switches.h"
#endif

using content::ContentRendererClient;
using content::RenderFrame;
using content::RenderView;

namespace bison {

BisonContentRendererClient::BisonContentRendererClient() {}

BisonContentRendererClient::~BisonContentRendererClient() {}

void BisonContentRendererClient::RenderThreadStarted() {
  web_cache_impl_.reset(new web_cache::WebCacheImpl());
}

void BisonContentRendererClient::RenderViewCreated(
    content::RenderView* render_view) {
  new BisonRenderViewObserver(render_view);
}

bool BisonContentRendererClient::HasErrorPage(int http_status_code) {
  return http_status_code >= 400;
}

bool BisonContentRendererClient::HandleNavigation(
    RenderFrame* render_frame,
    bool is_content_initiated,
    bool render_view_was_created_by_renderer,
    blink::WebFrame* frame,
    const blink::WebURLRequest& request,
    blink::WebNavigationType type,
    blink::WebNavigationPolicy default_policy,
    bool is_redirect) {
  VLOG(0) << "HandleNavigation";
  if (!request.HttpMethod().Equals("GET"))
    return false;

  return false;
}

void BisonContentRendererClient::PrepareErrorPage(
    RenderFrame* render_frame,
    const blink::WebURLError& error,
    const std::string& http_method,
    std::string* error_html) {
  if (error_html && error_html->empty()) {
    *error_html =
        "<head><title>Error</title></head><body>Could not load the requested "
        "resource.<br/>Error code: " +
        base::NumberToString(error.reason()) +
        (error.reason() < 0 ? " (" + net::ErrorToString(error.reason()) + ")"
                            : "") +
        "</body>";
  }
}

void BisonContentRendererClient::PrepareErrorPageForHttpStatusError(
    content::RenderFrame* render_frame,
    const GURL& unreachable_url,
    const std::string& http_method,
    int http_status,
    std::string* error_html) {
  if (error_html) {
    *error_html =
        "<head><title>Error</title></head><body>Server returned HTTP status " +
        base::NumberToString(http_status) + "</body>";
  }
}

bool BisonContentRendererClient::IsPluginAllowedToUseDevChannelAPIs() {
  return false;
}

void BisonContentRendererClient::DidInitializeWorkerContextOnWorkerThread(
    v8::Local<v8::Context> context) {}

#if BUILDFLAG(ENABLE_MOJO_CDM)
void BisonContentRendererClient::AddSupportedKeySystems(
    std::vector<std::unique_ptr<media::KeySystemProperties>>* key_systems) {
  if (!base::FeatureList::IsEnabled(media::kExternalClearKeyForTesting))
    return;

  static const char kExternalClearKeyKeySystem[] =
      "org.chromium.externalclearkey";
  key_systems->emplace_back(
      new cdm::ExternalClearKeyProperties(kExternalClearKeyKeySystem));
}
#endif

}  // namespace bison
