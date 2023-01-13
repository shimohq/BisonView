// create by jiang947

#ifndef BISON_RENDERER_BISON_KEY_SYSTEMS_H_
#define BISON_RENDERER_BISON_KEY_SYSTEMS_H_


#include <memory>
#include <vector>

#include "media/base/key_system_properties.h"

namespace bison {

void BvAddKeySystems(std::vector<std::unique_ptr<media::KeySystemInfo>>*
                         key_systems_properties);

}  // namespace bison

#endif  // BISON_RENDERER_BISON_KEY_SYSTEMS_H_
