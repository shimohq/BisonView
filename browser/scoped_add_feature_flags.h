// create by jiang947

#ifndef BISON_BROWSER_SCOPED_ADD_FEATURE_FLAGS_H_
#define BISON_BROWSER_SCOPED_ADD_FEATURE_FLAGS_H_

#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"

namespace base {
class CommandLine;
}

namespace bison {

class ScopedAddFeatureFlags {
 public:
  explicit ScopedAddFeatureFlags(base::CommandLine* cl);

  ScopedAddFeatureFlags(const ScopedAddFeatureFlags&) = delete;
  ScopedAddFeatureFlags& operator=(const ScopedAddFeatureFlags&) = delete;

  ~ScopedAddFeatureFlags();

  // Any existing (user set) enable/disable takes precedence.
  void EnableIfNotSet(const base::Feature& feature);
  void DisableIfNotSet(const base::Feature& feature);
  void EnableIfNotSetWithParameter(const base::Feature& feature,
                                   std::string name,
                                   std::string value);
  // Check if the feature is enabled from command line or functions above
  bool IsEnabled(const base::Feature& feature);
  bool IsEnabledWithParameter(const base::Feature& feature,
                              const std::string& name,
                              const std::string& value);

 private:
  void AddFeatureIfNotSet(const base::Feature& feature,
                          const std::string& suffix,
                          bool enable);

  const raw_ptr<base::CommandLine> cl_;
  std::vector<std::string> enabled_features_;
  std::vector<std::string> disabled_features_;
};

}  // namespace bison

#endif  // BISON_BROWSER_SCOPED_ADD_FEATURE_FLAGS_H_
