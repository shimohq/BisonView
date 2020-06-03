// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_CORE_BROWSER_TRACING_BISON_TRACING_DELEGATE_H_
#define BISON_CORE_BROWSER_TRACING_BISON_TRACING_DELEGATE_H_

#include <memory>

#include "content/public/browser/tracing_delegate.h"

namespace network {
class SharedURLLoaderFactory;
}

namespace bison {

class BisonTracingDelegate : public content::TracingDelegate {
 public:
  BisonTracingDelegate();
  ~BisonTracingDelegate() override;

  // content::TracingDelegate implementation:
  std::unique_ptr<content::TraceUploader> GetTraceUploader(
      scoped_refptr<network::SharedURLLoaderFactory> factory) override;
  std::unique_ptr<base::DictionaryValue> GenerateMetadataDict() override;
};

}  // namespace bison

#endif  // BISON_CORE_BROWSER_TRACING_BISON_TRACING_DELEGATE_H_
