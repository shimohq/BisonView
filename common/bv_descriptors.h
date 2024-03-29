// create by jiang947

#ifndef BISON_LIB_BISON_DESCRIPTORS_H_
#define BISON_LIB_BISON_DESCRIPTORS_H_

#include "content/public/common/content_descriptors.h"

// This is a list of global descriptor keys to be used with the
// base::GlobalDescriptors object (see base/posix/global_descriptors.h)
enum {
  kBisonViewLocalePakDescriptor = kContentIPCDescriptorMax + 1,
  kBisonViewMainPakDescriptor,
  kBisonView100PercentPakDescriptor,
  kAndroidWebViewCrashSignalDescriptor,
  kAndroidMinidumpDescriptor,
};

#endif  // BISON_LIB_BISON_DESCRIPTORS_H_
