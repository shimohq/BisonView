// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison_content_client.h"

#include "base/command_line.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "content/app/resources/grit/content_resources.h"
#include "content/public/common/content_switches.h"
// #include "content/shell/common/shell_switches.h"
#include "bison/grit/bison_resources.h"
#include "third_party/blink/public/strings/grit/blink_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace bison {

BisonContentClient::BisonContentClient() {}

BisonContentClient::~BisonContentClient() {}

base::string16 BisonContentClient::GetLocalizedString(int message_id) {
  return l10n_util::GetStringUTF16(message_id);
}

base::StringPiece BisonContentClient::GetDataResource(
    int resource_id,
    ui::ScaleFactor scale_factor) {
  return ui::ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

base::RefCountedMemory* BisonContentClient::GetDataResourceBytes(
    int resource_id) {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
      resource_id);
}

gfx::Image& BisonContentClient::GetNativeImageNamed(int resource_id) {
  return ui::ResourceBundle::GetSharedInstance().GetNativeImageNamed(
      resource_id);
}

base::DictionaryValue BisonContentClient::GetNetLogConstants() {
  base::DictionaryValue client_constants;
  client_constants.SetString("name", "bison");
  client_constants.SetString(
      "command_line",
      base::CommandLine::ForCurrentProcess()->GetCommandLineString());
  base::DictionaryValue constants;
  constants.SetKey("clientInfo", std::move(client_constants));
  return constants;
}

// blink::OriginTrialPolicy* BisonContentClient::GetOriginTrialPolicy() {
//   return &origin_trial_policy_;
// }

}  // namespace bison
