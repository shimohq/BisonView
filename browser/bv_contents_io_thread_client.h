// create by jiang947

#ifndef BISON_BROWSER_BISON_CONTENTS_IO_THREAD_CLIENT_H_
#define BISON_BROWSER_BISON_CONTENTS_IO_THREAD_CLIENT_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"

namespace content {
class WebContents;
}

namespace net {
class URLRequest;
}

namespace bison {

class BisonWebResourceInterceptResponse;
class BvWebResourceOverrideRequest;
struct BisonWebResourceRequest;

// This class provides a means of calling Java methods on an instance that has
// a 1:1 relationship with a WebContents instance directly from the IO thread.
//
// Specifically this is used to associate URLRequests with the WebContents that
// the URLRequest is made for.
//
// The native class is intended to be a short-lived handle that pins the
// Java-side instance. It is preferable to use the static getter methods to
// obtain a new instance of the class rather than holding on to one for
// prolonged periods of time (see note for more details).
//
// Note: The native BvContentsIoThreadClient instance has a Global ref to
// the Java object. By keeping the native BvContentsIoThreadClient
// instance alive you're also prolonging the lifetime of the Java instance, so
// don't keep a BvContentsIoThreadClient if you don't need to.
class BvContentsIoThreadClient {
 public:
  // Corresponds to WebSettings cache mode constants.
  enum CacheMode {
    LOAD_DEFAULT = -1,
    LOAD_NORMAL = 0,
    LOAD_CACHE_ELSE_NETWORK = 1,
    LOAD_NO_CACHE = 2,
    LOAD_CACHE_ONLY = 3,
  };

  // Called when BvContents is created before there is a Java client.
  static void RegisterPendingContents(content::WebContents* web_contents);

  // Associates the |jclient| instance (which must implement the
  // BvContentsIoThreadClient Java interface) with the |web_contents|.
  // This should be called at most once per |web_contents|.
  static void Associate(content::WebContents* web_contents,
                        const base::android::JavaRef<jobject>& jclient);

  // Sets the |jclient| java instance to which service worker related
  // callbacks should be delegated.
  static void SetServiceWorkerIoThreadClient(
      const base::android::JavaRef<jobject>& jclient,
      const base::android::JavaRef<jobject>& browser_context);

  // Either |pending_associate| is true or |jclient| holds a non-null
  // Java object.
  BvContentsIoThreadClient(bool pending_associate,
                              const base::android::JavaRef<jobject>& jclient);
  ~BvContentsIoThreadClient();

  // Implementation of BvContentsIoThreadClient.

  // Returns whether this is a new pop up that is still waiting for association
  // with the java counter part.
  bool PendingAssociation() const;

  // Retrieve CacheMode setting value of this BvContents.
  // This method is called on the IO thread only.
  CacheMode GetCacheMode() const;

  // This will attempt to fetch the BvContentsIoThreadClient for the given
  // |render_process_id|, |render_frame_id| pair.
  // This method can be called from any thread.
  // A null std::unique_ptr is a valid return value.
  static std::unique_ptr<BvContentsIoThreadClient> FromID(
      int render_process_id,
      int render_frame_id);

  // This map is useful when browser side navigations are enabled as
  // render_frame_ids will not be valid anymore for some of the navigations.
  static std::unique_ptr<BvContentsIoThreadClient> FromID(
      int frame_tree_node_id);

  // Returns the global thread client for service worker related callbacks.
  // A null std::unique_ptr is a valid return value.
  static std::unique_ptr<BvContentsIoThreadClient>
  GetServiceWorkerIoThreadClient();

  // Called on the IO thread when a subframe is created.
  static void SubFrameCreated(int render_process_id,
                              int parent_render_frame_id,
                              int child_render_frame_id);

  // This method is called on the IO thread only.
  using ShouldInterceptRequestResponseCallback = base::OnceCallback<void(
      std::unique_ptr<BisonWebResourceInterceptResponse>)>;
  using OverrideRequestRequestCallback = base::OnceCallback<void(
      std::unique_ptr<BvWebResourceOverrideRequest>)>;
  void ShouldInterceptRequestAsync(
      BisonWebResourceRequest request,
      ShouldInterceptRequestResponseCallback callback);

  void OverrideRequestHeaderAsync(
      BisonWebResourceRequest request,
      OverrideRequestRequestCallback callback);

  // Retrieve the AllowContentAccess setting value of this BvContents.
  // This method is called on the IO thread only.
  bool ShouldBlockContentUrls() const;

  // Retrieve the AllowFileAccess setting value of this BvContents.
  // This method is called on the IO thread only.
  bool ShouldBlockFileUrls() const;

  // Retrieve the BlockNetworkLoads setting value of this BvContents.
  // This method is called on the IO thread only.
  bool ShouldBlockNetworkLoads() const;

  // Retrieve the AcceptThirdPartyCookies setting value of this BvContents.
  bool ShouldAcceptThirdPartyCookies() const;

  // Retrieve the SafeBrowsingEnabled setting value of this BvContents.
  bool GetSafeBrowsingEnabled() const;

 private:
  bool pending_association_;
  base::android::ScopedJavaGlobalRef<jobject> java_object_;
  base::android::ScopedJavaGlobalRef<jobject> bg_thread_client_object_;
  scoped_refptr<base::SequencedTaskRunner> sequenced_task_runner_ =
      base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()});

  DISALLOW_COPY_AND_ASSIGN(BvContentsIoThreadClient);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_CONTENTS_IO_THREAD_CLIENT_H_
