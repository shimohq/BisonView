

#include <string>

#include "bison/browser/bv_media_url_interceptor.h"
#include "bison/common/url_constants.h"
#include "base/android/apk_assets.h"
#include "base/strings/string_util.h"
#include "content/public/common/url_constants.h"

namespace bison {

bool BvMediaUrlInterceptor::Intercept(const std::string& url,
                                      int* fd,
                                      int64_t* offset,
                                      int64_t* size) const {
  std::string asset_file_prefix(url::kFileScheme);
  asset_file_prefix.append(url::kStandardSchemeSeparator);
  asset_file_prefix.append(bison::kAndroidAssetPath);

  if (base::StartsWith(url, asset_file_prefix, base::CompareCase::SENSITIVE)) {
    std::string filename(url);
    base::ReplaceFirstSubstringAfterOffset(&filename, 0, asset_file_prefix,
                                           "assets/");
    base::MemoryMappedFile::Region region =
        base::MemoryMappedFile::Region::kWholeFile;
    *fd = base::android::OpenApkAsset(filename, &region);
    *offset = region.offset;
    *size = region.size;
    return *fd != -1;
  }

  return false;
}

}  // namespace bison
