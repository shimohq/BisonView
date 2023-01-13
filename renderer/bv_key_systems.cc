
#include "bison/renderer/bv_key_systems.h"
#include "components/cdm/renderer/android_key_systems.h"

namespace bison {

void BvAddKeySystems(std::vector<std::unique_ptr<media::KeySystemInfo>>*
                         key_systems_properties) {
#if BUILDFLAG(ENABLE_WIDEVINE)
  cdm::AddAndroidWidevine(key_systems_properties);
#endif  // BUILDFLAG(ENABLE_WIDEVINE)
  cdm::AddAndroidPlatformKeySystems(key_systems_properties);
}

}  // namespace bison
