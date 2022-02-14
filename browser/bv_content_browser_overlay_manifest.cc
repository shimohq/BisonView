#include "bison/browser/bv_content_browser_overlay_manifest.h"

#include "base/no_destructor.h"
#include "bison/common/js_java_interaction/interfaces.mojom.h"
#include "content/public/common/service_names.mojom.h"
#include "services/service_manager/public/cpp/manifest_builder.h"
#include "third_party/blink/public/mojom/input/input_host.mojom.h"

namespace bison {
const service_manager::Manifest& GetContentBrowserOverlayManifest() {
  static base::NoDestructor<service_manager::Manifest> manifest{
      service_manager::ManifestBuilder()
          .ExposeInterfaceFilterCapability_Deprecated(
              "navigation:frame", "renderer",
              service_manager::Manifest::InterfaceList<
                  blink::mojom::TextSuggestionHost, mojom::JsToJavaMessaging>())
          .Build()};
  return *manifest;
}

}  // namespace bison
