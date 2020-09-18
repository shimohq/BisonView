#include "bison/browser/bison_content_renderer_overlay_manifest.h"

#include "base/no_destructor.h"
#include "bison/common/js_java_interaction/interfaces.mojom.h"
#include "components/safe_browsing/common/safe_browsing.mojom.h"
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
                  // autofill::mojom::AutofillAgent,
                  // autofill::mojom::PasswordAutofillAgent,
                  // autofill::mojom::PasswordGenerationAgent,
                  safe_browsing::mojom::ThreatReporter,
                  mojom::JsJavaConfigurator, mojom::JsToJavaMessaging>())
          // v8 .bin
          .PreloadFile(content::kV8NativesDataDescriptor,
                       base::FilePath(
                           FILE_PATH_LITERAL("assets/bison_natives_blob.bin")))
          .PreloadFile(
              content::kV8Snapshot32DataDescriptor,
              base::FilePath(FILE_PATH_LITERAL("assets/bison_snapshot_blob_32.bin")))
          .PreloadFile(
              content::kV8Snapshot64DataDescriptor,
              base::FilePath(FILE_PATH_LITERAL("assets/bison_snapshot_blob_64.bin")))
          .Build()};
  return *manifest;
}

}  // namespace bison
