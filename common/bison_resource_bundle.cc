#include "bison/common/bison_resource_bundle.h"

#include "bison/common/bison_descriptors.h"

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

  auto* global_descriptors = base::GlobalDescriptors::GetInstance();
  int pak_fd = global_descriptors->MaybeGet(kBisonPakDescriptor);
  base::MemoryMappedFile::Region pak_region;
  if (pak_fd >= 0) {
    pak_region = global_descriptors->GetRegion(kBisonPakDescriptor);
  } else {
    pak_fd = base::android::OpenApkAsset("assets/bison.pak", &pak_region);
  }
  global_descriptors->Set(kBisonPakDescriptor, pak_fd, pak_region);
}

void InitResourceBundleRendererSide() {
  auto* global_descriptors = base::GlobalDescriptors::GetInstance();
  int pak_fd = global_descriptors->Get(kBisonPakDescriptor);
  base::MemoryMappedFile::Region pak_region =
      global_descriptors->GetRegion(kBisonPakDescriptor);
  ui::ResourceBundle::InitSharedInstanceWithPakFileRegion(base::File(pak_fd),
                                                          pak_region);
  ui::ResourceBundle::GetSharedInstance().AddDataPackFromFileRegion(
      base::File(pak_fd), pak_region, ui::SCALE_FACTOR_100P);
  // std::pair<int, ui::ScaleFactor> extra_paks[] = {{ui::SCALE_FACTOR_100P}};

  // for (const auto& pak_info : extra_paks) {
  //   pak_fd = global_descriptors->Get(pak_info.first);
  //   pak_region = global_descriptors->GetRegion(pak_info.first);
  //   ui::ResourceBundle::GetSharedInstance().AddDataPackFromFileRegion(
  //       base::File(pak_fd), pak_region, pak_info.second);
  // }
}

}  // namespace bison
