// create by jiang947 


#ifndef BISON_BROWSER_BISON_VARIATIONS_SEED_BRIDGE_H_
#define BISON_BROWSER_BISON_VARIATIONS_SEED_BRIDGE_H_

#include <memory>
#include <string>

#include "components/variations/seed_response.h"

namespace bison {

// If the Java side has a seed, return it and clear it from the Java side.
// Otherwise, return null.
std::unique_ptr<variations::SeedResponse> GetAndClearJavaSeed();

// Returns true if the variations seed that was loaded is fresh.
bool IsSeedFresh();

}  // namespace bison

#endif  // BISON_BROWSER_BISON_VARIATIONS_SEED_BRIDGE_H_
