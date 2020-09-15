// create by jiang947 

#ifndef BISON_BROWSER_BISON_CONTENT_RENDERER_OVERLAY_MANIFEST_H_
#define BISON_BROWSER_BISON_CONTENT_RENDERER_OVERLAY_MANIFEST_H_

#include "services/service_manager/public/cpp/manifest.h"

namespace bison {

// Returns the manifest Android WebView amends to Content's content_renderer
// service manifest. This allows WebView to extend the capabilities exposed
// and/or required by content_renderer service instances.
const service_manager::Manifest& GetContentRendererOverlayManifest();

}  // namespace bison

#endif  // BISON_BROWSER_BISON_CONTENT_RENDERER_OVERLAY_MANIFEST_H_
