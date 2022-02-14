#include "bison/common/bv_resource_bundle.h"

#include "bison/common/bv_descriptors.h"

#include "base/android/apk_assets.h"
#include "base/android/locale_utils.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/memory_mapped_file.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/posix/global_descriptors.h"
#include "base/trace_event/trace_event.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/resource/resource_bundle_android.h"
#include "ui/base/ui_base_paths.h"

namespace bison {

void InitIcuAndResourceBundleBrowserSide() {
  TRACE_EVENT0("startup", "InitIcuAndResourceBundleBrowserSide");
  ui::SetLocalePaksStoredInApk(true);
  std::string locale = ui::ResourceBundle::InitSharedInstanceWithLocale(
      base::android::GetDefaultLocaleString(), NULL,
      ui::ResourceBundle::LOAD_COMMON_RESOURCES);
  if (locale.empty()) {
    LOG(WARNING) << "Failed to load locale .pak from apk.";
  }
  base::i18n::SetICUDefaultLocale(locale);


  base::FilePath pak_file_path;
  base::PathService::Get(ui::DIR_RESOURCE_PAKS_ANDROID, &pak_file_path);
  VLOG(0) << "pak_file_path:" << pak_file_path;
  pak_file_path = pak_file_path.AppendASCII("resources.pak");
  ui::LoadMainAndroidPackFile("assets/bison/resources.pak", pak_file_path);
}

void InitResourceBundleRendererSide() {
  auto* global_descriptors = base::GlobalDescriptors::GetInstance();
  int pak_fd = global_descriptors->Get(kBisonViewLocalePakDescriptor);
  base::MemoryMappedFile::Region pak_region =
      global_descriptors->GetRegion(kBisonViewLocalePakDescriptor);
  ui::ResourceBundle::InitSharedInstanceWithPakFileRegion(base::File(pak_fd),
                                                          pak_region);

  std::pair<int, ui::ScaleFactor> extra_paks[] = {
      {kBisonViewMainPakDescriptor, ui::SCALE_FACTOR_NONE},
      {kBisonView100PercentPakDescriptor, ui::SCALE_FACTOR_100P}};

  for (const auto& pak_info : extra_paks) {
    pak_fd = global_descriptors->Get(pak_info.first);
    pak_region = global_descriptors->GetRegion(pak_info.first);
    ui::ResourceBundle::GetSharedInstance().AddDataPackFromFileRegion(
        base::File(pak_fd), pak_region, pak_info.second);
  }
}

}  // namespace bison
