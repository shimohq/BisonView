
#ifndef BISON_BROWSER_VARIATIONS_VARIATIONS_SEED_LOADER_H_
#define BISON_BROWSER_VARIATIONS_VARIATIONS_SEED_LOADER_H_

#include <memory>

namespace bison {

class BvVariationsSeed;

std::unique_ptr<BvVariationsSeed> TakeSeed();

}  // namespace bison

#endif  // BISON_BROWSER_VARIATIONS_VARIATIONS_SEED_LOADER_H_
