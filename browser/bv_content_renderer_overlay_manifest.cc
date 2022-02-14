#include "bison/browser/bv_content_renderer_overlay_manifest.h"

#include "base/no_destructor.h"
#include "bison/common/js_java_interaction/interfaces.mojom.h"
#include "content/public/common/content_descriptor_keys.h"
#include "content/public/common/service_names.mojom.h"
#include "services/service_manager/public/cpp/manifest_builder.h"

namespace bison {

const service_manager::Manifest& GetContentRendererOverlayManifest() {
  static base::NoDestructor<service_manager::Manifest> manifest{
      service_manager::ManifestBuilder()
          .ExposeInterfaceFilterCapability_Deprecated(
              "navigation:frame", "browser",
              service_manager::Manifest::InterfaceList<
                  mojom::JsJavaConfigurator, mojom::JsToJavaMessaging>())
          .Build()};
  return *manifest;
}

}  // namespace bison
