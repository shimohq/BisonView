// create by jiang947


#ifndef BISON_BROWSER_BISON_WEB_UI_CONTROLLER_FACTORY_H_
#define BISON_BROWSER_BISON_WEB_UI_CONTROLLER_FACTORY_H_


#include "base/macros.h"
#include "base/memory/singleton.h"
#include "content/public/browser/web_ui_controller_factory.h"

namespace bison {

class BvWebUIControllerFactory : public content::WebUIControllerFactory {
 public:
  static BvWebUIControllerFactory* GetInstance();

  // content::WebUIControllerFactory overrides
  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) override;
  bool UseWebUIForURL(content::BrowserContext* browser_context,
                      const GURL& url) override;
  bool UseWebUIBindingsForURL(content::BrowserContext* browser_context,
                              const GURL& url) override;
  std::unique_ptr<content::WebUIController> CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) override;

 private:
  friend struct base::DefaultSingletonTraits<BvWebUIControllerFactory>;

  BvWebUIControllerFactory();
  ~BvWebUIControllerFactory() override;

  DISALLOW_COPY_AND_ASSIGN(BvWebUIControllerFactory);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_WEB_UI_CONTROLLER_FACTORY_H_
