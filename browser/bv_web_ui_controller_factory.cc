
#include "bison/browser/bv_web_ui_controller_factory.h"

#include "base/memory/ptr_util.h"
// #include "components/safe_browsing/web_ui/constants.h"
// #include "components/safe_browsing/web_ui/safe_browsing_ui.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "url/gurl.h"

using content::WebUI;
using content::WebUIController;

namespace {

const WebUI::TypeID kSafeBrowsingID = &kSafeBrowsingID;

// A function for creating a new WebUI. The caller owns the return value, which
// may be nullptr (for example, if the URL refers to an non-existent extension).
typedef WebUIController* (*WebUIFactoryFunctionPointer)(WebUI* web_ui,
                                                        const GURL& url);

// Template for defining WebUIFactoryFunctionPointer.
template <class T>
WebUIController* NewWebUI(WebUI* web_ui, const GURL& url) {
  return new T(web_ui);
}

WebUIFactoryFunctionPointer GetWebUIFactoryFunctionPointer(const GURL& url) {
  return nullptr;
}

WebUI::TypeID GetWebUITypeID(const GURL& url) {
  return WebUI::kNoWebUI;
}

}  // namespace

namespace bison {

// static
BvWebUIControllerFactory* BvWebUIControllerFactory::GetInstance() {
  return base::Singleton<BvWebUIControllerFactory>::get();
}

BvWebUIControllerFactory::BvWebUIControllerFactory() {}

BvWebUIControllerFactory::~BvWebUIControllerFactory() {}

WebUI::TypeID BvWebUIControllerFactory::GetWebUIType(
    content::BrowserContext* browser_context,
    const GURL& url) {
  return GetWebUITypeID(url);
}

bool BvWebUIControllerFactory::UseWebUIForURL(
    content::BrowserContext* browser_context,
    const GURL& url) {
  return GetWebUIType(browser_context, url) != WebUI::kNoWebUI;
}

bool BvWebUIControllerFactory::UseWebUIBindingsForURL(
    content::BrowserContext* browser_context,
    const GURL& url) {
  return UseWebUIForURL(browser_context, url);
}

std::unique_ptr<WebUIController>
BvWebUIControllerFactory::CreateWebUIControllerForURL(WebUI* web_ui,
                                                      const GURL& url) {
  WebUIFactoryFunctionPointer function = GetWebUIFactoryFunctionPointer(url);
  if (!function)
    return nullptr;

  return base::WrapUnique((*function)(web_ui, url));
}

}  // namespace bison
