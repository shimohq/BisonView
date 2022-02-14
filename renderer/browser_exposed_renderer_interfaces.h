// create by jiang947


#ifndef BISON_RENDERER_BROWSER_EXPOSED_RENDERER_INTERFACES_H_
#define BISON_RENDERER_BROWSER_EXPOSED_RENDERER_INTERFACES_H_


namespace mojo {
class BinderMap;
}  // namespace mojo

namespace bison {

class BvContentRendererClient;

void ExposeRendererInterfacesToBrowser(BvContentRendererClient* client,
                                       mojo::BinderMap* binders);

}  // namespace bison

#endif  // BISON_RENDERER_BROWSER_EXPOSED_RENDERER_INTERFACES_H_
