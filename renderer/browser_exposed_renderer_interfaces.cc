// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/renderer/browser_exposed_renderer_interfaces.h"

#include "bison/renderer/bv_content_renderer_client.h"

#include "base/threading/thread_task_runner_handle.h"
#include "components/visitedlink/renderer/visitedlink_reader.h"
#include "mojo/public/cpp/bindings/binder_map.h"

namespace bison {

void ExposeRendererInterfacesToBrowser(BvContentRendererClient* client,
                                       mojo::BinderMap* binders) {
  binders->Add<visitedlink::mojom::VisitedLinkNotificationSink>(
      client->visited_link_reader()->GetBindCallback(),
               base::ThreadTaskRunnerHandle::Get());
}

}  // namespace bison
