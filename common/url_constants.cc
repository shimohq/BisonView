#include "url_constants.h"

#include "base/strings/string_util.h"

namespace bison {

// http://developer.android.com/reference/android/webkit/WebSettings.html
const char kAndroidAssetPath[] = "/android_asset/";
const char kAndroidResourcePath[] = "/android_res/";

const char kAndroidWebViewVideoPosterScheme[] = "android-webview-video-poster";

bool IsAndroidSpecialFileUrl(const GURL& url) {
  if (!url.is_valid() || !url.SchemeIsFile() || !url.has_path())
    return false;
  return base::StartsWith(url.path(), kAndroidAssetPath,
                          base::CompareCase::SENSITIVE) ||
         base::StartsWith(url.path(), kAndroidResourcePath,
                          base::CompareCase::SENSITIVE);
}

}  // namespace bison
