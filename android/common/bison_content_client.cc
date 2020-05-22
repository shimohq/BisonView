// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison_content_client.h"

#include "bison_media_drm_bridge_client.h"
#include "bison_resource.h"
#include "crash_reporter/crash_keys.h"
#include "url_constants.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/debug/crash_logging.h"
#include "base/no_destructor.h"
#include "components/services/heap_profiling/public/cpp/profiling_client.h"
#include "components/version_info/version_info.h"
#include "content/public/common/content_switches.h"
#include "gpu/config/gpu_info.h"
#include "gpu/config/gpu_util.h"
#include "ipc/ipc_message.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace bison {

void BisonContentClient::AddAdditionalSchemes(Schemes* schemes) {
  schemes->local_schemes.push_back(url::kContentScheme);
  schemes->secure_schemes.push_back(
      bison::kAndroidWebViewVideoPosterScheme);
  schemes->allow_non_standard_schemes_in_origins = true;
}

base::string16 BisonContentClient::GetLocalizedString(int message_id) {
  // TODO(boliu): Used only by WebKit, so only bundle those resources for
  // Android WebView.
  return l10n_util::GetStringUTF16(message_id);
}

base::StringPiece BisonContentClient::GetDataResource(
    int resource_id,
    ui::ScaleFactor scale_factor) {
  // TODO(boliu): Used only by WebKit, so only bundle those resources for
  // Android WebView.
  return ui::ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

base::RefCountedMemory* BisonContentClient::GetDataResourceBytes(int resource_id) {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
      resource_id);
}

bool BisonContentClient::CanSendWhileSwappedOut(const IPC::Message* message) {
  // For legacy API support we perform a few browser -> renderer synchronous IPC
  // messages that block the browser. However, the synchronous IPC replies might
  // be dropped by the renderer during a swap out, deadlocking the browser.
  // Because of this we should never drop any synchronous IPC replies.
  return message->type() == IPC_REPLY_ID;
}

void BisonContentClient::SetGpuInfo(const gpu::GPUInfo& gpu_info) {
  gpu_fingerprint_ = gpu_info.gl_version + '|' + gpu_info.gl_vendor + '|' +
                     gpu_info.gl_renderer;
  std::replace_if(gpu_fingerprint_.begin(), gpu_fingerprint_.end(),
                  [](char c) { return !::isprint(c); }, '_');

  gpu::SetKeysForCrashLogging(gpu_info);
}

bool BisonContentClient::UsingSynchronousCompositing() {
  return true;
}

media::MediaDrmBridgeClient* BisonContentClient::GetMediaDrmBridgeClient() {
  // return new BisonMediaDrmBridgeClient(
  //     BisonResource::GetConfigKeySystemUuidMapping());
  // TODO jiang 先屏幕 或者参考 chrome/common/chrome_content_client.cc
  return nullptr;
}

void BisonContentClient::BindChildProcessInterface(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* receiving_handle) {
  // This creates a process-wide heap_profiling::ProfilingClient that listens
  // for requests from the HeapProfilingService to start profiling the current
  // process.
  static base::NoDestructor<heap_profiling::ProfilingClient> profiling_client;
  if (interface_name == heap_profiling::ProfilingClient::Name_) {
    profiling_client->BindToInterface(
        mojo::PendingReceiver<heap_profiling::mojom::ProfilingClient>(
            std::move(*receiving_handle)));
  }
}

}  // namespace bison
