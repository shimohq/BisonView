

#include "bison_devtools_frontend.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "bison_browser_context.h"
#include "bison_devtools_bindings.h"
#include "bison_devtools_manager_delegate.h"
#include "bison_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"

namespace bison {

namespace {
static GURL GetFrontendURL() {
  int port = BisonDevToolsManagerDelegate::GetHttpHandlerPort();
  return GURL(base::StringPrintf(
      "http://127.0.0.1:%d/devtools/devtools_app.html", port));
}
}  // namespace

// static
BisonDevToolsFrontend* BisonDevToolsFrontend::Show(
    WebContents* inspected_contents) {
  BisonView* bison_view = BisonView::CreateNewWindow(
      inspected_contents->GetBrowserContext(), GURL(), nullptr, gfx::Size());
  BisonDevToolsFrontend* devtools_frontend =
      new BisonDevToolsFrontend(bison_view, inspected_contents);
  bison_view->LoadURL(GetFrontendURL());
  return devtools_frontend;
}

void BisonDevToolsFrontend::Activate() {
  frontend_bison_view_->ActivateContents(frontend_bison_view_->web_contents());
}

void BisonDevToolsFrontend::Focus() {
  frontend_bison_view_->web_contents()->Focus();
}

void BisonDevToolsFrontend::InspectElementAt(int x, int y) {
  // frontend_bison_view_->InspectElementAt(x, y);
}

void BisonDevToolsFrontend::Close() {
  frontend_bison_view_->Close();
}

void BisonDevToolsFrontend::DocumentAvailableInMainFrame() {
  // frontend_bison_view_->Attach();
}

void BisonDevToolsFrontend::WebContentsDestroyed() {
  delete this;
}

BisonDevToolsFrontend::BisonDevToolsFrontend(BisonView* frontend_bison_view,
                                             WebContents* inspected_contents)
    : WebContentsObserver(frontend_bison_view->web_contents()),
      frontend_bison_view_(frontend_bison_view),
      devtools_bindings_(
          new BisonDevToolsBindings(frontend_bison_view->web_contents(),
                                    inspected_contents,
                                    this)) {}

BisonDevToolsFrontend::~BisonDevToolsFrontend() {}

}  // namespace bison
