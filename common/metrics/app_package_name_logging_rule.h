#ifndef BISON_COMMON_METRICS_APP_PACKAGE_NAME_LOGGING_RULE_H_
#define BISON_COMMON_METRICS_APP_PACKAGE_NAME_LOGGING_RULE_H_

#include "base/time/time.h"
#include "base/values.h"
#include "base/version.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace bison {

// A class to hold the state of whether an app package name should be recorded
// in UMA metrics log or not. This represent the result of looking the app
// package name in a list of allowed apps.
class AppPackageNameLoggingRule {
 public:
  AppPackageNameLoggingRule(const base::Version& version,
                            const base::Time& expiry_date);
  ~AppPackageNameLoggingRule() = default;

  AppPackageNameLoggingRule(const AppPackageNameLoggingRule&) = default;
  AppPackageNameLoggingRule& operator=(const AppPackageNameLoggingRule&) =
      default;
  AppPackageNameLoggingRule(AppPackageNameLoggingRule&&) = default;
  AppPackageNameLoggingRule& operator=(AppPackageNameLoggingRule&&) = default;

  base::Version GetVersion() const;
  base::Time GetExpiryDate() const;

  // Return `true` is the app is in the allowlist and the result hasn't expired,
  // `false` otherwise.
  bool IsAppPackageNameAllowed() const;

  // If it has the same version and expiry_date as `record`.
  bool IsSameAs(const AppPackageNameLoggingRule& record) const;

  base::Value ToDictionary();

  // Creates a valid AppPackageNameLoggingRule from a dictionary, or null if
  // the dictionary have invalid values.
  static absl::optional<AppPackageNameLoggingRule> FromDictionary(
      const base::Value& dict);

 private:
  base::Version version_;
  base::Time expiry_date_;
};

}  // namespace bison

#endif  // BISON_COMMON_METRICS_APP_PACKAGE_NAME_LOGGING_RULE_H_
