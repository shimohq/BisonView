// create by jiang947

#ifndef BISON_BROWSER_BISON_RESOURCE_CONTEXT_H_
#define BISON_BROWSER_BISON_RESOURCE_CONTEXT_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "content/public/browser/resource_context.h"

class GURL;

namespace bison {

class BvResourceContext : public content::ResourceContext {
 public:
  BvResourceContext();
  ~BvResourceContext() override;

  void SetExtraHeaders(const GURL& url, const std::string& headers);
  std::string GetExtraHeaders(const GURL& url);

 private:
  base::Lock extra_headers_lock_;
  std::map<std::string, std::string> extra_headers_;

  DISALLOW_COPY_AND_ASSIGN(BvResourceContext);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_RESOURCE_CONTEXT_H_
