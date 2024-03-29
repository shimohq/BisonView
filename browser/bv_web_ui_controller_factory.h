// create by jiang947

#ifndef BISON_BROWSER_BV_WEB_UI_CONTROLLER_FACTORY_H_
#define BISON_BROWSER_BV_WEB_UI_CONTROLLER_FACTORY_H_



#include "base/memory/singleton.h"
#include "content/public/browser/web_ui_controller_factory.h"

namespace bison {

class BvWebUIControllerFactory : public content::WebUIControllerFactory {
 public:
  static BvWebUIControllerFactory* GetInstance();

  BvWebUIControllerFactory(const BvWebUIControllerFactory&) = delete;
  BvWebUIControllerFactory& operator=(const BvWebUIControllerFactory&) = delete;

  // content::WebUIControllerFactory overrides
  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) override;
  bool UseWebUIForURL(content::BrowserContext* browser_context,
                      const GURL& url) override;
  std::unique_ptr<content::WebUIController> CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) override;

 private:
  friend struct base::DefaultSingletonTraits<BvWebUIControllerFactory>;

  BvWebUIControllerFactory();
  ~BvWebUIControllerFactory() override;


};

}  // namespace bison

#endif  // BISON_BROWSER_BV_WEB_UI_CONTROLLER_FACTORY_H_
