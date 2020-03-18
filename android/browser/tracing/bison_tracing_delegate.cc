// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/browser/tracing/bison_tracing_delegate.h"

#include <memory>

#include "base/logging.h"
#include "base/values.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/trace_uploader.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace bison {

BisonTracingDelegate::BisonTracingDelegate() {}
BisonTracingDelegate::~BisonTracingDelegate() {}

std::unique_ptr<content::TraceUploader> BisonTracingDelegate::GetTraceUploader(
    scoped_refptr<network::SharedURLLoaderFactory>) {
  NOTREACHED();
  return NULL;
}

std::unique_ptr<base::DictionaryValue>
BisonTracingDelegate::GenerateMetadataDict() {
  auto metadata_dict = std::make_unique<base::DictionaryValue>();
  metadata_dict->SetString("revision", version_info::GetLastChange());
  return metadata_dict;
}

}  // namespace bison
