// create by jiang947

#ifndef BISON_COMMON_URL_CONSTANTS_H_
#define BISON_COMMON_URL_CONSTANTS_H_

#include "url/gurl.h"

namespace bison {

extern const char kAndroidAssetPath[];
extern const char kAndroidResourcePath[];
// Returns whether the given URL is for loading a file from a special path.
bool IsAndroidSpecialFileUrl(const GURL& url);

extern const char kAndroidWebViewVideoPosterScheme[];

}  // namespace bison

#endif  // BISON_COMMON_URL_CONSTANTS_H_
