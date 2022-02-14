// create by jiang947

#ifndef BISON_BROWSER_NETWORK_SERVICE_NET_HELPERS_H_
#define BISON_BROWSER_NETWORK_SERVICE_NET_HELPERS_H_

#include <memory>

class GURL;

namespace bison {

class BvContentsIoThreadClient;

// Returns the updated request's |load_flags| based on the settings.
int UpdateLoadFlags(int load_flags, BvContentsIoThreadClient* client);

// Returns true if the given URL should be aborted with
// net::ERR_ACCESS_DENIED.
bool ShouldBlockURL(const GURL& url, BvContentsIoThreadClient* client);

// Determines the desired size for WebView's on-disk HttpCache, measured in
// Bytes.
int GetHttpCacheSize();

}  // namespace bison

#endif  // BISON_BROWSER_NETWORK_SERVICE_NET_HELPERS_H_
