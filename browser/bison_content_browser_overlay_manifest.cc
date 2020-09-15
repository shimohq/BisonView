#include "bison/browser/bison_content_browser_overlay_manifest.h"

#include "bison/common/js_java_interaction/interfaces.mojom.h"
#include "base/no_destructor.h"
#include "components/safe_browsing/common/safe_browsing.mojom.h"
#include "components/spellcheck/common/spellcheck.mojom.h"
#include "content/public/common/service_names.mojom.h"
#include "services/service_manager/public/cpp/manifest_builder.h"
#include "third_party/blink/public/mojom/input/input_host.mojom.h"

namespace bison {

const service_manager::Manifest& GetContentBrowserOverlayManifest() {
  // jiang unuse spellcheck::mojom::SpellCheckHost
  static base::NoDestructor<service_manager::Manifest> manifest{
      service_manager::ManifestBuilder()
          .ExposeCapability("renderer",
                            service_manager::Manifest::InterfaceList<
                                safe_browsing::mojom::SafeBrowsing>())
          .ExposeInterfaceFilterCapability_Deprecated(
              "navigation:frame", "renderer",
              service_manager::Manifest::InterfaceList<
                  blink::mojom::TextSuggestionHost, mojom::JsToJavaMessaging>())
          .Build()};
  return *manifest;
}

}  // namespace bison
