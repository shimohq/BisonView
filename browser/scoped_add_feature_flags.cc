#include "bison/browser/scoped_add_feature_flags.h"

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/strings/string_util.h"

namespace bison {

ScopedAddFeatureFlags::ScopedAddFeatureFlags(base::CommandLine* cl) : cl_(cl) {
  std::string enabled_features =
      cl->GetSwitchValueASCII(switches::kEnableFeatures);
  std::string disabled_features =
      cl->GetSwitchValueASCII(switches::kDisableFeatures);
  for (auto& sp : base::FeatureList::SplitFeatureListString(enabled_features))
    enabled_features_.emplace_back(sp);
  for (auto& sp : base::FeatureList::SplitFeatureListString(disabled_features))
    disabled_features_.emplace_back(sp);
}

ScopedAddFeatureFlags::~ScopedAddFeatureFlags() {
  cl_->AppendSwitchASCII(switches::kEnableFeatures,
                         base::JoinString(enabled_features_, ","));
  cl_->AppendSwitchASCII(switches::kDisableFeatures,
                         base::JoinString(disabled_features_, ","));
}

void ScopedAddFeatureFlags::EnableIfNotSet(const base::Feature& feature) {
  AddFeatureIfNotSet(feature, /*suffix=*/"", /*enable=*/true);
}

void ScopedAddFeatureFlags::EnableIfNotSetWithParameter(
    const base::Feature& feature,
    std::string name,
    std::string value) {
  std::string suffix = ":" + name + "/" + value;
  AddFeatureIfNotSet(feature, suffix, true /* enable */);
}

void ScopedAddFeatureFlags::DisableIfNotSet(const base::Feature& feature) {
  AddFeatureIfNotSet(feature, /*suffix=*/"", /*enable=*/false);
}

bool ScopedAddFeatureFlags::IsEnabled(const base::Feature& feature) {
  return IsEnabledWithParameter(feature, /*name=*/"", /*value=*/"");
}

bool ScopedAddFeatureFlags::IsEnabledWithParameter(const base::Feature& feature,
                                                   const std::string& name,
                                                   const std::string& value) {
  std::string feature_name = feature.name;
  if (!name.empty()) {
    feature_name += ":" + name + "/" + value;
  }
  if (base::Contains(disabled_features_, feature_name))
    return false;
  if (base::Contains(enabled_features_, feature_name))
    return true;
  return feature.default_state == base::FEATURE_ENABLED_BY_DEFAULT;
}

void ScopedAddFeatureFlags::AddFeatureIfNotSet(const base::Feature& feature,
                                               const std::string& suffix,
                                               bool enable) {
  std::string feature_name = feature.name;
  feature_name += suffix;
  if (base::Contains(enabled_features_, feature_name) ||
      base::Contains(disabled_features_, feature_name)) {
    return;
  }
  if (enable) {
    enabled_features_.emplace_back(feature_name);
  } else {
    disabled_features_.emplace_back(feature_name);
  }
}

}  // namespace bison
