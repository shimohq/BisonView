// create by jiang947 


#ifndef BISON_BROWSER_NETWORK_SERVICE_BISON_PROXYING_URL_LOADER_FACTORY_H_
#define BISON_BROWSER_NETWORK_SERVICE_BISON_PROXYING_URL_LOADER_FACTORY_H_

#include "bison/browser/network_service/android_stream_reader_url_loader.h"

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace net {
struct MutableNetworkTrafficAnnotationTag;
}

namespace network {
struct ResourceRequest;
}

namespace bison {

// URL Loader Factory for Android WebView. This is the entry point for handling
// Android WebView callbacks (i.e. error, interception and other callbacks) and
// loading of android specific schemes and overridden responses.
//
// This class contains centralized logic for:
//  - request interception and blocking,
//  - setting load flags and headers,
//  - loading requests depending on the scheme (e.g. different delegates are
//    used for loading android assets/resources as compared to overridden
//    responses).
//  - handling errors (e.g. no input stream, redirect or safebrowsing related
//    errors).
//
// In particular handles the following Android WebView callbacks:
//  - shouldInterceptRequest
//  - onReceivedError
//  - onReceivedHttpError
//  - onReceivedLoginRequest
//
// Threading:
//  Currently the factory and the associated loader assume they live on the IO
//  thread. This is also required by the shouldInterceptRequest callback (which
//  should be called on a non-UI thread). The other callbacks (i.e.
//  onReceivedError, onReceivedHttpError and onReceivedLoginRequest) are posted
//  on the UI thread.
//
class BisonProxyingURLLoaderFactory : public network::mojom::URLLoaderFactory {
 public:
   using SecurityOptions = AndroidStreamReaderURLLoader::SecurityOptions;

  // Create a factory that will create specialized URLLoaders for Android
  // WebView. If |intercept_only| parameter is true the loader created by
  // this factory will only execute the intercept callback
  // (shouldInterceptRequest), it will not propagate the request to the
  // target factory.
  BisonProxyingURLLoaderFactory(
      int process_id,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> loader_receiver,
      mojo::PendingRemote<network::mojom::URLLoaderFactory>
          target_factory_remote,
      bool intercept_only,
      base::Optional<SecurityOptions> security_options);

  ~BisonProxyingURLLoaderFactory() override;

  // static
  static void CreateProxy(
      int process_id,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> loader,
      mojo::PendingRemote<network::mojom::URLLoaderFactory>
          target_factory_remote,
      base::Optional<SecurityOptions> security_options);

  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override;

  void Clone(mojo::PendingReceiver<network::mojom::URLLoaderFactory>
                 loader_receiver) override;

 private:
  void OnTargetFactoryError();
  void OnProxyBindingError();

  const int process_id_;
  mojo::ReceiverSet<network::mojom::URLLoaderFactory> proxy_receivers_;
  mojo::Remote<network::mojom::URLLoaderFactory> target_factory_;

  // When true the loader resulting from this factory will only execute
  // intercept callback (shouldInterceptRequest). If that returns without
  // a response, the loader will abort loading.
  bool intercept_only_;

  base::Optional<SecurityOptions> security_options_;

  base::WeakPtrFactory<BisonProxyingURLLoaderFactory> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(BisonProxyingURLLoaderFactory);
};

}  // namespace bison

#endif  // BISON_BROWSER_NETWORK_SERVICE_BISON_PROXYING_URL_LOADER_FACTORY_H_
