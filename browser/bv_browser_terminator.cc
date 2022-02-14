#include "bison/browser/bv_browser_terminator.h"

#include <unistd.h>
#include <memory>

#include "bison/browser/bv_render_process_gone_delegate.h"
#include "bison/common/bv_descriptors.h"

#include "base/android/scoped_java_ref.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/browser/child_process_launcher_utils.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_iterator.h"
#include "content/public/browser/web_contents.h"

using base::android::ScopedJavaGlobalRef;
using content::BrowserThread;

namespace bison {

namespace {

void GetJavaWebContentsForRenderProcess(
    content::RenderProcessHost* rph,
    std::vector<ScopedJavaGlobalRef<jobject>>* java_web_contents) {
  std::unique_ptr<content::RenderWidgetHostIterator> widgets(
      content::RenderWidgetHost::GetRenderWidgetHosts());
  while (content::RenderWidgetHost* widget = widgets->GetNextHost()) {
    content::RenderViewHost* view = content::RenderViewHost::From(widget);
    if (view && rph == view->GetProcess()) {
      content::WebContents* wc = content::WebContents::FromRenderViewHost(view);
      if (wc) {
        java_web_contents->push_back(static_cast<ScopedJavaGlobalRef<jobject>>(
            wc->GetJavaWebContents()));
      }
    }
  }
}

void OnRenderProcessGone(
    const std::vector<ScopedJavaGlobalRef<jobject>>& java_web_contents,
    base::ProcessId child_process_pid,
    bool crashed) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  for (auto& java_wc : java_web_contents) {
    content::WebContents* wc =
        content::WebContents::FromJavaWebContents(java_wc);
    if (!wc)
      continue;

    BvRenderProcessGoneDelegate* delegate =
        BvRenderProcessGoneDelegate::FromWebContents(wc);
    if (!delegate)
      continue;

    switch (delegate->OnRenderProcessGone(child_process_pid, crashed)) {
      case BvRenderProcessGoneDelegate::RenderProcessGoneResult::kException:
        // Let the exception propagate back to the message loop.
        base::MessageLoopCurrentForUI::Get()->Abort();
        return;
      case BvRenderProcessGoneDelegate::RenderProcessGoneResult::kUnhandled:
        if (crashed) {
          // Keeps this log unchanged, CTS test uses it to detect crash.
          std::string message = base::StringPrintf(
              "Render process (%d)'s crash wasn't handled by all associated  "
              "bisonviews, triggering application crash.",
              child_process_pid);
        } else {
          // The render process was most likely killed for OOM or switching
          // WebView provider, to make WebView backward compatible, kills the
          // browser process instead of triggering crash.
          LOG(ERROR) << "Render process (" << child_process_pid << ") kill (OOM"
                     << " or update) wasn't handed by all associated bisonviews,"
                     << " killing application.";
          kill(getpid(), SIGKILL);
        }
        NOTREACHED();
        break;
      case BvRenderProcessGoneDelegate::RenderProcessGoneResult::kHandled:
        break;
    }
  }

}

}  // namespace

BvBrowserTerminator::BvBrowserTerminator() = default;

BvBrowserTerminator::~BvBrowserTerminator() = default;

void BvBrowserTerminator::OnChildExit(
    const crash_reporter::ChildExitObserver::TerminationInfo& info) {
  content::RenderProcessHost* rph =
      content::RenderProcessHost::FromID(info.process_host_id);

  if (info.normal_termination) {
    return;
  }

  LOG(ERROR) << "Renderer process (" << info.pid << ") crash detected (code "
             << info.crash_signo << ").";

  std::vector<ScopedJavaGlobalRef<jobject>> java_web_contents;
  GetJavaWebContentsForRenderProcess(rph, &java_web_contents);

  content::GetUIThreadTaskRunner({base::TaskPriority::HIGHEST})
      ->PostTask(FROM_HERE,
                 base::BindOnce(OnRenderProcessGone, java_web_contents,
                                info.pid, info.is_crashed()));
}

}  // namespace bison
