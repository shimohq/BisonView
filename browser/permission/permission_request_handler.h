// create by jiang947

#ifndef BISON_BROWSER_PERMISSION_PERMISSION_REQUEST_HANDLER_H_
#define BISON_BROWSER_PERMISSION_PERMISSION_REQUEST_HANDLER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "url/gurl.h"

namespace bison {

class BvPermissionRequest;
class BvPermissionRequestDelegate;
class PermissionRequestHandlerClient;

// This class is used to send the permission requests, or cancel ongoing
// requests.
// It is owned by BvContents and has 1x1 mapping to BvContents. All methods
// are running on UI thread.
class PermissionRequestHandler : public content::WebContentsObserver {
 public:
  PermissionRequestHandler(PermissionRequestHandlerClient* client,
                           content::WebContents* bison_contents);
  ~PermissionRequestHandler() override;

  // Send the given |request| to PermissionRequestHandlerClient.
  void SendRequest(std::unique_ptr<BvPermissionRequestDelegate> request);

  // Cancel the ongoing request initiated by |origin| for accessing |resources|.
  void CancelRequest(const GURL& origin, int64_t resources);

  // Allow |origin| to access the |resources|.
  void PreauthorizePermission(const GURL& origin, int64_t resources);

  // WebContentsObserver
  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override;

 private:
  friend class TestPermissionRequestHandler;

  typedef std::vector<base::WeakPtr<BvPermissionRequest>>::iterator
      RequestIterator;

  // Return the request initiated by |origin| for accessing |resources|.
  RequestIterator FindRequest(const GURL& origin, int64_t resources);

  // Cancel the given request.
  void CancelRequestInternal(RequestIterator i);

  void CancelAllRequests();

  // Remove the invalid requests from requests_.
  void PruneRequests();

  // Return true if |origin| were preauthorized to access |resources|.
  bool Preauthorized(const GURL& origin, int64_t resources);

  PermissionRequestHandlerClient* client_;

  // A list of ongoing requests.
  std::vector<base::WeakPtr<BvPermissionRequest>> requests_;

  std::map<std::string, int64_t> preauthorized_permission_;

  // The unique id of the active NavigationEntry of the WebContents that we were
  // opened for. Used to help expire on requests.
  int contents_unique_id_;


};

}  // namespace bison

#endif  // BISON_BROWSER_PERMISSION_PERMISSION_REQUEST_HANDLER_H_
