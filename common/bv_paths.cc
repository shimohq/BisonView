#include "bison/common/bv_paths.h"

#include "base/android/path_utils.h"
#include "base/base_paths_android.h"
#include "base/files/file_util.h"
#include "base/path_service.h"

namespace bison {

bool PathProvider(int key, base::FilePath* result) {
  base::FilePath cur;

  switch (key) {
    case DIR_CRASH_DUMPS:
      if (!base::android::GetCacheDirectory(&cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("Crashpad"));
      break;
    default:
      return false;
  }

  *result = cur;
  return true;
}

void RegisterPathProvider() {
  base::PathService::RegisterProvider(PathProvider, PATH_START, PATH_END);
}

}  // namespace bison
