#ifndef BISON_BROWSER_BV_CONTENTS_ORIGIN_MATCHER_H_
#define BISON_BROWSER_BV_CONTENTS_ORIGIN_MATCHER_H_


#include <memory>
#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"

namespace url {
class Origin;
}

namespace js_injection {
class OriginMatcher;
}

namespace bison {

// Wrapper for a |js_incection::OriginMatcher| that allows locked updates
// to the match rules.
class BvContentsOriginMatcher
    : public base::RefCountedThreadSafe<BvContentsOriginMatcher> {
 public:
  BvContentsOriginMatcher();
  bool MatchesOrigin(const url::Origin& origin);
  // Returns the list of invalid rules.
  // If there are bad rules, no update is performed
  std::vector<std::string> UpdateRuleList(
      const std::vector<std::string>& rules);

 private:
  friend class base::RefCountedThreadSafe<BvContentsOriginMatcher>;
  ~BvContentsOriginMatcher();

  base::Lock lock_;
  std::unique_ptr<js_injection::OriginMatcher> origin_matcher_;
};

}  // namespace bison

#endif  // BISON_BROWSER_BV_CONTENTS_ORIGIN_MATCHER_H_
