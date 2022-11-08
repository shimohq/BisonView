#ifndef BISON_COMMON_COMPONENTS_BV_APPS_PACKAGE_NAMES_ALLOWLIST_COMPONENT_UTILS_H_
#define BISON_COMMON_COMPONENTS_BV_APPS_PACKAGE_NAMES_ALLOWLIST_COMPONENT_UTILS_H_

#include <stdint.h>

#include <vector>

namespace bison {

extern const char kWebViewAppsPackageNamesAllowlistComponentId[];

void GetWebViewAppsPackageNamesAllowlistPublicKeyHash(
    std::vector<uint8_t>* hash);

}  // namespace bison

#endif  // BISON_COMMON_COMPONENTS_BV_APPS_PACKAGE_NAMES_ALLOWLIST_COMPONENT_UTILS_H_
