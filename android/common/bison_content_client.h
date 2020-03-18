// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_COMMON_BISON_CONTENT_CLIENT_H_
#define BISON_ANDROID_COMMON_BISON_CONTENT_CLIENT_H_

#include "content/public/common/content_client.h"

#include "base/compiler_specific.h"

namespace gpu {
struct GPUInfo;
}

namespace bison {

class BisonContentClient : public content::ContentClient {
 public:
  // ContentClient implementation.
  void AddAdditionalSchemes(Schemes* schemes) override;
  base::string16 GetLocalizedString(int message_id) override;
  base::StringPiece GetDataResource(int resource_id,
                                    ui::ScaleFactor scale_factor) override;
  base::RefCountedMemory* GetDataResourceBytes(int resource_id) override;
  bool CanSendWhileSwappedOut(const IPC::Message* message) override;
  void SetGpuInfo(const gpu::GPUInfo& gpu_info) override;
  bool UsingSynchronousCompositing() override;
  media::MediaDrmBridgeClient* GetMediaDrmBridgeClient() override;
  void BindChildProcessInterface(
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* receiving_handle) override;

  const std::string& gpu_fingerprint() const { return gpu_fingerprint_; }

 private:
  std::string gpu_fingerprint_;
};

}  // namespace bison

#endif  // BISON_ANDROID_COMMON_BISON_CONTENT_CLIENT_H_
