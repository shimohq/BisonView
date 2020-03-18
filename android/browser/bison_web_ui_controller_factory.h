// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BISON_ANDROID_BROWSER_BISON_WEB_UI_CONTROLLER_FACTORY_H_
#define BISON_ANDROID_BROWSER_BISON_WEB_UI_CONTROLLER_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "content/public/browser/web_ui_controller_factory.h"

namespace bison {

class BisonWebUIControllerFactory : public content::WebUIControllerFactory {
 public:
  static BisonWebUIControllerFactory* GetInstance();

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
  friend struct base::DefaultSingletonTraits<BisonWebUIControllerFactory>;

  BisonWebUIControllerFactory();
  ~BisonWebUIControllerFactory() override;

  DISALLOW_COPY_AND_ASSIGN(BisonWebUIControllerFactory);
};

}  // namespace bison

#endif  // BISON_ANDROID_BROWSER_BISON_WEB_UI_CONTROLLER_FACTORY_H_
