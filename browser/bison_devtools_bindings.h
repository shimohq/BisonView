// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_SHELL_DEVTOOLS_BINDINGS_H_
#define CONTENT_SHELL_BROWSER_SHELL_DEVTOOLS_BINDINGS_H_

#include <memory>
#include <set>

#include "base/compiler_specific.h"
#include "base/containers/unique_ptr_adapters.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/web_contents_observer.h"

#if !defined(OS_ANDROID)
#include "content/public/browser/devtools_frontend_host.h"
#endif

namespace base {
class Value;
}

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

using content::DevToolsAgentHost;
using content::DevToolsAgentHostClient;
using content::WebContents;
using content::WebContentsObserver;
using content::NavigationHandle;

namespace bison {

class BisonDevToolsDelegate {
 public:
  virtual void Close() = 0;
  virtual ~BisonDevToolsDelegate() {}
};

class BisonDevToolsBindings : public WebContentsObserver,
                              public DevToolsAgentHostClient {
 public:
  BisonDevToolsBindings(WebContents* devtools_contents,
                        WebContents* inspected_contents,
                        BisonDevToolsDelegate* delegate);

  static std::vector<BisonDevToolsBindings*> GetInstancesForWebContents(
      WebContents* web_contents);

  void InspectElementAt(int x, int y);
  virtual void Attach();
  void UpdateInspectedWebContents(WebContents* new_contents);

  void CallClientFunction(const std::string& function_name,
                          const base::Value* arg1,
                          const base::Value* arg2,
                          const base::Value* arg3);
  ~BisonDevToolsBindings() override;

  WebContents* inspected_contents() { return inspected_contents_; }

 private:
  // content::DevToolsAgentHostClient implementation.
  void AgentHostClosed(DevToolsAgentHost* agent_host) override;
  void DispatchProtocolMessage(DevToolsAgentHost* agent_host,
                               const std::string& message) override;

  void HandleMessageFromDevToolsFrontend(const std::string& message);

  // WebContentsObserver overrides
  void ReadyToCommitNavigation(NavigationHandle* navigation_handle) override;
  void WebContentsDestroyed() override;

  void SendMessageAck(int request_id, const base::Value* arg1);
  void AttachInternal();

  WebContents* inspected_contents_;
  BisonDevToolsDelegate* delegate_;
  scoped_refptr<DevToolsAgentHost> agent_host_;
  int inspect_element_at_x_;
  int inspect_element_at_y_;
#if !defined(OS_ANDROID)
  std::unique_ptr<DevToolsFrontendHost> frontend_host_;
#endif

  class NetworkResourceLoader;
  std::set<std::unique_ptr<NetworkResourceLoader>, base::UniquePtrComparator>
      loaders_;

  base::DictionaryValue preferences_;

  using ExtensionsAPIs = std::map<std::string, std::string>;
  ExtensionsAPIs extensions_api_;
  base::WeakPtrFactory<BisonDevToolsBindings> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(BisonDevToolsBindings);
};

}  // namespace bison

#endif  // CONTENT_SHELL_BROWSER_SHELL_DEVTOOLS_BINDINGS_H_
